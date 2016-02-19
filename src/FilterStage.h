//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_FILTERBLOCK_H
#define MATCHIT_FILTERBLOCK_H

#include <string>
#include "./Stage.h"
#include "./CodegenUtils.h"
#include "./ForLoop.h"

template <typename I>
class FilterStage : public Stage {

private:

    bool (*filter)(const I*);

public:

    FilterStage(bool (*filter)(const I*), std::string filter_name, JIT *jit, MType *param_type) :
            Stage(jit, "FilterStage", filter_name, param_type, param_type, MPrimType::get_bool_type()),
            filter(filter) { }

    ~FilterStage() {}

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    bool is_filter() {
        return true;
    }

    void codegen() {
        if (!codegen_done) {
            init_codegen();

            // load the inputs to the wrapper function
            WrapperArgLoaderIB wal;
            wal.insert(mfunction->get_extern_wrapper());
            wal.codegen(jit);
            ExternArgLoaderIB eal;
            eal.insert(mfunction->get_extern_wrapper());

            ForLoop loop(jit, mfunction->get_extern_wrapper());
            llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy",
                                                               mfunction->get_extern_wrapper());
            jit->get_builder().CreateBr(loop.get_counter_bb());

            // loop counters
            jit->get_builder().SetInsertPoint(loop.get_counter_bb());
            llvm::AllocaInst *counters[3];
            loop.codegen_counters(counters, 3);
            llvm::AllocaInst *loop_idx = counters[0];
            llvm::AllocaInst *loop_bound = counters[1];
            llvm::AllocaInst *ret_idx = counters[2];
            jit->get_builder().CreateStore(
                    jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 1]), loop_bound);

            llvm::AllocaInst *num_prim_values_ctr; // only used if this isn't fixed size
            // preallocate space for the output
            // get the output type underneath the pointer it is wrapper in
            // matched size, i.e. the number of primitive values in each output Element's array is allocated to be the same size as the input array
            num_prim_values_ctr = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 2]),
                                           num_prim_values_ctr); // this field contains the number of primitive values, which is static even though
            llvm::LoadInst *loop_bound_load = jit->get_builder().CreateLoad(loop_bound);
            // across structs the sizes are variable
            llvm::AllocaInst *space = mfunction->get_extern_param_types()[0]->get_underlying_types()[0]->
                    preallocate_matched_block(jit, loop_bound_load, jit->get_builder().CreateLoad(num_prim_values_ctr),
                                              mfunction->get_extern_wrapper(), wal.get_args_alloc()[0], true);
            jit->get_builder().CreateBr(loop.get_condition_bb());

            // loop condition
            loop.codegen_condition(loop_bound, loop_idx);
            jit->get_builder().CreateCondBr(loop.get_condition()->get_loop_comparison(), eal.get_basic_block(), dummy);

            // loop increment
            loop.codegen_increment(loop_idx);
            jit->get_builder().CreateBr(loop.get_condition_bb());

            // loop body
            // get the inputs to the extern function
            eal.set_loop_idx_alloc(loop_idx);
            eal.set_wrapper_input_arg_alloc(wal.get_args_alloc()[0]);
            eal.codegen(jit); // don't set preallocated output space b/c the user isn't getting an output passed to their function
            // call the extern function
            ExternCallIB ec;
            ec.insert(mfunction->get_extern_wrapper());
            ec.set_extern_function(mfunction->get_extern());
            jit->get_builder().CreateBr(ec.get_basic_block());
            ec.set_extern_arg_allocs(eal.get_extern_input_arg_alloc());
            ec.codegen(jit);
            // in the filter stage, the inputs are the outputs, so copy those over if the output of the extern call is true
            llvm::LoadInst *call_res = jit->get_builder().CreateLoad(ec.get_extern_call_result_alloc());
            llvm::Value *cmp = jit->get_builder().CreateICmpEQ(call_res, CodegenUtils::get_i1(0)); // compare to false
            llvm::BasicBlock *store_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store",
                                                                     mfunction->get_extern_wrapper());
            jit->get_builder().CreateCondBr(cmp, loop.get_increment_bb(), store_block);

            // Store the result
            jit->get_builder().SetInsertPoint(store_block);
            // get the output space
            llvm::LoadInst *output_load = jit->get_builder().CreateLoad(space);
            llvm::LoadInst *ret_idx_load = jit->get_builder().CreateLoad(ret_idx);
            std::vector<llvm::Value *> output_struct_idxs;
            output_struct_idxs.push_back(ret_idx_load);
            llvm::Value *output_gep = jit->get_builder().CreateInBoundsGEP(output_load, output_struct_idxs);
            // get the input
            llvm::LoadInst *input_load = jit->get_builder().CreateLoad(eal.get_extern_input_arg_alloc()[0]);
            // copy the input pointer to the output pointer
            jit->get_builder().CreateStore(input_load, output_gep);
            // increment ret idx
            llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(ret_idx_load, CodegenUtils::get_i64(1));
            jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
            jit->get_builder().CreateBr(loop.get_increment_bb());

            // finish up the stage and allocate space for the outputs
            jit->get_builder().SetInsertPoint(dummy);
            llvm::AllocaInst *wrapper_result = jit->get_builder().CreateAlloca(
                    mfunction->get_extern_wrapper_return_type()->codegen_type());
            // RetStruct *rs = (RetStruct*)malloc(sizeof(RetStruct)), RetStruct = { { i64, i64, float* }**, i64, i64 }, so sizeof = 24
            llvm::Value *return_space = CodegenUtils::codegen_c_malloc64_and_cast(jit, 24,
                                                                                  mfunction->get_extern_wrapper_return_type()->codegen_type());
            jit->get_builder().CreateStore(return_space, wrapper_result);
            llvm::LoadInst *temp_wrapper_result_load = jit->get_builder().CreateLoad(wrapper_result);

            // store the preallocated space in the output struct (these gep idxs are for the different fields in the output struct)
            llvm::Value *field_one = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 0); // the data elements
            llvm::Value *field_two = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 1); // num prim values across all the data elements
            llvm::Value *field_three = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 2); // num data elements

            // store all the processed structs in field one
            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(space), field_one);
            // store the number of primitive values across all the structs
            llvm::Value *num_prim_values;
            num_prim_values = jit->get_builder().CreateLoad(num_prim_values_ctr);
            jit->get_builder().CreateStore(num_prim_values, field_two);
            // store the number of structs being returned
            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(ret_idx), field_three);
            jit->get_builder().CreateRet(jit->get_builder().CreateLoad(wrapper_result));
            codegen_done = true;
        }
    }


    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternArgLoaderIB *eal,
                                ExternCallIB *ec, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {

//        // build the body
//        eal->set_mfunction(mfunction);
//        eal->set_loop_idx_alloc(loop_idx);
//        eal->set_wrapper_input_arg_alloc(args[0]);
//        eal->codegen(jit, false);
//        jit->get_builder().CreateBr(ec->get_basic_block());
//
//        // create the call
//        std::vector<llvm::AllocaInst *> sliced;
//        sliced.push_back(eal->get_extern_input_arg_alloc());
//        ec->set_mfunction(mfunction);
//        ec->set_extern_arg_allocs(sliced);
//        ec->codegen(jit, false);
//        llvm::LoadInst *load = jit->get_builder().CreateLoad(ec->get_extern_call_result_alloc());//get_secondary_extern_call_result_alloc());
//        std::vector<llvm::Value *> bool_gep_idx;
//        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(load, CodegenUtils::get_i1(0));
//        ec->set_secondary_extern_call_result_alloc(eal->get_extern_input_arg_alloc());
//        jit->get_builder().CreateCondBr(cmp, loop->get_for_loop_increment_basic_block()->get_basic_block(), branch_to);

    }

};

#endif //MATCHIT_FILTERBLOCK_H
