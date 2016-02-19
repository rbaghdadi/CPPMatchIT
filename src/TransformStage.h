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

    TransformStage(void (*transform)(const I*, O*), std::string transform_name, JIT *jit, MType *param_type,
                   MType *return_type, unsigned int transform_size, bool is_fixed_transform_size) :
            Stage(jit, "TransformStage", transform_name, param_type, return_type, MPrimType::get_void_type()),
            transform(transform), transform_size(transform_size), is_fixed_transform_size(is_fixed_transform_size) { }

    ~TransformStage() { }

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    bool is_transform() {
        return true;
    }

    unsigned int get_transform_size() {
        return transform_size;
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
            llvm::AllocaInst *counters[2];
            loop.codegen_counters(counters, 2);
            llvm::AllocaInst *loop_idx = counters[0];
            llvm::AllocaInst *loop_bound = counters[1];
            jit->get_builder().CreateStore(
                    jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 1]), loop_bound);

            llvm::AllocaInst *num_prim_values_ctr; // only used if this isn't fixed size
            // preallocate space for the output
            // get the output type underneath the pointer it is wrapper in
            llvm::AllocaInst *space;
            if (is_fixed_transform_size) {
                llvm::LoadInst *loop_bound_load = jit->get_builder().CreateLoad(loop_bound);
                space = mfunction->get_extern_param_types()[1]->get_underlying_types()[0]->preallocate_fixed_block(
                        jit, loop_bound_load,
                        jit->get_builder().CreateMul(loop_bound_load, CodegenUtils::get_i64(transform_size)),
                        CodegenUtils::get_i64(transform_size),
                        mfunction->get_extern_wrapper()); // the output element is the second argument to our extern function
            } else { // matched size, i.e. the number of primitive values in each output Element's array is allocated to be the same size as the input array
                num_prim_values_ctr = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
                jit->get_builder().CreateStore(
                        jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 2]),
                        num_prim_values_ctr); // this field contains the number of primitive values, which is static even though
                llvm::LoadInst *loop_bound_load = jit->get_builder().CreateLoad(loop_bound);
                // across structs the sizes are variable
                space = mfunction->get_extern_param_types()[1]->get_underlying_types()[0]->
                        preallocate_matched_block(jit, loop_bound_load, jit->get_builder().CreateLoad(num_prim_values_ctr),
                                                  mfunction->get_extern_wrapper(), wal.get_args_alloc()[0], false);
            }
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
            eal.set_preallocated_output_space(space);
            eal.set_wrapper_input_arg_alloc(wal.get_args_alloc()[0]);
            eal.codegen(jit);
            // call the extern function
            ExternCallIB ec;
            ec.insert(mfunction->get_extern_wrapper());
            ec.set_extern_function(mfunction->get_extern());
            jit->get_builder().CreateBr(ec.get_basic_block());
            ec.set_extern_arg_allocs(eal.get_extern_input_arg_alloc());
            ec.codegen(jit);
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
            // store all the processed structs in field one
            llvm::Value *field_one = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 0);
            llvm::Value *field_two = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 1);
            llvm::Value *field_three = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 2);
            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(space), field_one);
            // store the number of primitive values across all the structs
            llvm::Value *num_prim_values;
            if (is_fixed_transform_size) {
                num_prim_values = jit->get_builder().CreateMul(CodegenUtils::get_i64(transform_size),
                                                               jit->get_builder().CreateLoad(loop_idx));
            } else {
                num_prim_values = jit->get_builder().CreateLoad(num_prim_values_ctr);
            }
            jit->get_builder().CreateStore(num_prim_values, field_two);
            // store the number of structs being returned
            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(loop_idx), field_three);
            jit->get_builder().CreateRet(jit->get_builder().CreateLoad(wrapper_result));
            codegen_done = true;
        }
    }

};

#endif //MATCHIT_TRANSFORMSTAGE_H
