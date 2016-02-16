//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMSTAGE_H
#define MATCHIT_TRANSFORMSTAGE_H

#include "./Stage.h"
#include "./MFunc.h"
#include "./MType.h"
#include "./CodegenUtils.h"

template <typename I, typename O>
class TransformStage : public Stage {

private:

    void (*transform)(const I*, O*);
    unsigned int transform_size;
    bool is_fixed_transform_size = true;

public:

//    TransformStage(O (*transform)(I), std::string transform_name, JIT *jit, MType *param_type, MType *return_type) :
//            Stage(jit, mtype_of<I>(), mtype_of<O>(), transform_name), transform(transform) {
//        std::vector<MType *> param_types;
//        param_types.push_back(param_type);
//        MType *stage_return_type = new MPointerType(new WrapperOutputType(return_type));
//        MFunc *func = new MFunc(function_name, "TransformStage", new MPointerType(return_type), stage_return_type,
//                                param_types, jit);
//        set_function(func);
//    }

    TransformStage(void (*transform)(const I*, O*), std::string transform_name, JIT *jit, MType *param_type, MType *return_type, unsigned int transform_size, bool is_fixed_transform_size) :
            Stage(jit, mtype_of<I>(), mtype_of<O>(), transform_name), transform(transform), transform_size(transform_size), is_fixed_transform_size(is_fixed_transform_size) {
        std::vector<MType *> extern_param_types;
        extern_param_types.push_back(new MPointerType(param_type));
        extern_param_types.push_back(new MPointerType(return_type));
        std::vector<MType *> extern_wrapper_param_types;
        extern_wrapper_param_types.push_back(new MPointerType(new MPointerType(param_type)));
        extern_wrapper_param_types.push_back(new MPrimType(mtype_long, 64)); // the number of data elements coming in
        extern_wrapper_param_types.push_back(new MPrimType(mtype_long, 64)); // the total size of the arrays in the data elements coming in
        std::vector<MType *> return_types;
        MType *stage_return_type = new MPointerType(new MPointerType(return_type));//new MPointerType(new WrapperOutputType(return_type));
        return_types.push_back(stage_return_type); // the actual data type passed back
        return_types.push_back(new MPrimType(mtype_long, 64)); // the number of data elements going out
        return_types.push_back(new MPrimType(mtype_long, 64)); // the total size of the arrays in the data elements coming in
        MPointerType *combined_return_type = new MPointerType(new MStructType(mtype_struct, return_types));
        MFunc *func = new MFunc(function_name, "TransformStage", new MPointerType(return_type), combined_return_type,
                                extern_param_types, extern_wrapper_param_types, jit);
        set_function(func);
    }

    ~TransformStage() { }

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    void codegen() {

        init_codegen();

        // load the inputs to the wrapper function
        WrapperArgLoaderIB wal;
        wal.set_mfunction(mfunction);
        wal.codegen(jit);
        ExternArgLoaderIB eal;
        eal.set_mfunction(mfunction);

        llvm::BasicBlock *loop_counter = llvm::BasicBlock::Create(llvm::getGlobalContext(), "loop_counters", mfunction->get_extern_wrapper());
        llvm::BasicBlock *loop_condition = llvm::BasicBlock::Create(llvm::getGlobalContext(), "loop_condition", mfunction->get_extern_wrapper());
        llvm::BasicBlock *loop_increment = llvm::BasicBlock::Create(llvm::getGlobalContext(), "loop_increment", mfunction->get_extern_wrapper());
        llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy", mfunction->get_extern_wrapper());
        jit->get_builder().CreateBr(loop_counter);

        // various loop indices, counters
        jit->get_builder().SetInsertPoint(loop_counter);
        llvm::Type *counter_type = llvm::Type::getInt64Ty(llvm::getGlobalContext());
        llvm::AllocaInst *loop_idx = jit->get_builder().CreateAlloca(counter_type); // main loop index
        jit->get_builder().CreateStore(CodegenUtils::get_i64(0), loop_idx);
        llvm::AllocaInst *loop_bound = jit->get_builder().CreateAlloca(counter_type); // upper bound on the loop
        jit->get_builder().CreateStore(jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 1]), loop_bound);
        llvm::AllocaInst *num_prim_values_ctr; // only used if this isn't fixed size
        // preallocate space for the output
        // get the output type underneath the pointer it is wrapper in
        llvm::AllocaInst *space;
        if (is_fixed_transform_size) {
            llvm::LoadInst *loop_bound_load = jit->get_builder().CreateLoad(loop_bound);
            space = mfunction->get_extern_param_types()[1]->get_underlying_types()[0]->preallocate_fixed_block(
                    jit, loop_bound_load, jit->get_builder().CreateMul(loop_bound_load, CodegenUtils::get_i64(transform_size)),
                    CodegenUtils::get_i64(transform_size), mfunction->get_extern_wrapper()); // the output element is the second argument to our extern function
        } else { // matched size, i.e. the number of primitive values in each output Element's array is allocated to be the same size as the input array
            // TODO finished off here. Need to give
            num_prim_values_ctr = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 2]),
                                           num_prim_values_ctr); // this field contains the number of primitive values, which is static even though
            llvm::LoadInst *loop_bound_load = jit->get_builder().CreateLoad(loop_bound);
            // across structs the sizes are variable
            space = mfunction->get_extern_param_types()[1]->get_underlying_types()[0]->preallocate_matched_block(
                    jit, loop_bound_load,
                    jit->get_builder().CreateLoad(num_prim_values_ctr),
                    mfunction->get_extern_wrapper(), wal.get_args_alloc()[0]);
        }
        jit->get_builder().CreateBr(loop_condition);

        // comparison
        jit->get_builder().SetInsertPoint(loop_condition);
        llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
        llvm::LoadInst *bound = jit->get_builder().CreateLoad(loop_bound);
        llvm::Value *cmp = jit->get_builder().CreateICmpSLT(cur_loop_idx, bound);
        jit->get_builder().CreateCondBr(cmp, eal.get_basic_block(), dummy);

        // loop increment
        jit->get_builder().SetInsertPoint(loop_increment);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(loop_idx);
        llvm::Value *inc = jit->get_builder().CreateAdd(load, CodegenUtils::get_i64(1));
        jit->get_builder().CreateStore(inc, loop_idx);
        jit->get_builder().CreateBr(loop_condition);

        // loop body
        // get the inputs to the extern function
        eal.set_loop_idx_alloc(loop_idx);
        eal.set_preallocated_output_space(space);
        eal.set_wrapper_input_arg_alloc(wal.get_args_alloc()[0]);
        eal.codegen(jit);
        // call the extern function
        ExternCallIB ec;
        ec.set_mfunction(mfunction);
        jit->get_builder().CreateBr(ec.get_basic_block());
        ec.set_extern_arg_allocs(eal.get_extern_input_arg_alloc());
        ec.codegen(jit);
        jit->get_builder().CreateBr(loop_increment);

        // finish up the stage and allocate space for the outputs
        jit->get_builder().SetInsertPoint(dummy);
        llvm::AllocaInst *wrapper_result = jit->get_builder().CreateAlloca(mfunction->get_extern_wrapper_return_type()->codegen_type());
        // RetStruct *rs = (RetStruct*)malloc(sizeof(RetStruct)), RetStruct = { { i64, i64, float* }**, i64, i64 }, so sizeof = 24
        llvm::Value *return_space = CodegenUtils::codegen_c_malloc64_and_cast(jit, 24, mfunction->get_extern_wrapper_return_type()->codegen_type());
        jit->get_builder().CreateStore(return_space, wrapper_result);
        llvm::LoadInst *temp_wrapper_result_load = jit->get_builder().CreateLoad(wrapper_result);

        // store the preallocated space
        std::vector<llvm::Value *> one_gep_idxs; // the outputs
        std::vector<llvm::Value *> two_gep_idxs; // the num_prim_counter field
        std::vector<llvm::Value *> three_gep_idxs; // the num_elements field
        one_gep_idxs.push_back(CodegenUtils::get_i32(0));
        one_gep_idxs.push_back(CodegenUtils::get_i32(0));
        two_gep_idxs.push_back(CodegenUtils::get_i32(0));
        two_gep_idxs.push_back(CodegenUtils::get_i32(1));
        three_gep_idxs.push_back(CodegenUtils::get_i32(0));
        three_gep_idxs.push_back(CodegenUtils::get_i32(2));

        // store all the processed structs in field one
        llvm::Value *field_one_gep = jit->get_builder().CreateInBoundsGEP(temp_wrapper_result_load, one_gep_idxs);
        jit->get_builder().CreateStore(jit->get_builder().CreateLoad(space), field_one_gep);
        // store the number of primitive values across all the structs
        llvm::Value *num_prim_values;
        if (is_fixed_transform_size) {
            num_prim_values = jit->get_builder().CreateMul(CodegenUtils::get_i64(transform_size),
                                                                        jit->get_builder().CreateLoad(loop_idx));
        } else {
            num_prim_values = jit->get_builder().CreateLoad(num_prim_values_ctr);
        }
        llvm::Value *field_two_gep = jit->get_builder().CreateInBoundsGEP(temp_wrapper_result_load, two_gep_idxs);
        jit->get_builder().CreateStore(num_prim_values, field_two_gep);
        // store the number of structs being returned
        llvm::Value *field_three_gep = jit->get_builder().CreateInBoundsGEP(temp_wrapper_result_load, three_gep_idxs);
        jit->get_builder().CreateStore(jit->get_builder().CreateLoad(loop_idx), field_three_gep);
        jit->get_builder().CreateRet(jit->get_builder().CreateLoad(wrapper_result));
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
//        jit->get_builder().CreateBr(branch_to);

    }

};

#endif //MATCHIT_TRANSFORMSTAGE_H
