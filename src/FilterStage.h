//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_FILTERBLOCK_H
#define MATCHIT_FILTERBLOCK_H

#include <string>
#include "./Stage.h"
#include "./CodegenUtils.h"
#include "./ForLoop.h"

using namespace CodegenUtils;

class FilterStage : public Stage {

private:

    bool (*filter)(const SetElement * const);

public:

    FilterStage(bool (*filter)(const SetElement * const), std::string filter_name, JIT *jit, Relation *input_relation) :
            Stage(jit, "FilterStage", filter_name, input_relation, input_relation, MScalarType::get_bool_type()),
            filter(filter) { }

//                MType *param_type) :
//            Stage(jit, "FilterStage", filter_name, param_type, param_type, MScalarType::get_bool_type()),
//            filter(filter) { }

    ~FilterStage() {}

    bool is_filter() {
        return true;
    }

    void handle_extern_output(llvm::AllocaInst *output_data_array_size) {
        llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_extern_call_result_alloc(), 8);
        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(call_result_load, get_i1(0)); // compare to false
        llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy",
                                                           mfunction->get_extern_wrapper());
        jit->get_builder().CreateCondBr(cmp, loop->get_increment_bb(), dummy);
        jit->get_builder().SetInsertPoint(dummy);
        llvm::LoadInst *output_load = codegen_llvm_load(jit, user_function_arg_loader->get_preallocated_output_space(), 8);
        llvm::LoadInst *ret_idx_load = codegen_llvm_load(jit, loop->get_return_idx(), 4);
        std::vector<llvm::Value *> output_struct_idxs;
        output_struct_idxs.push_back(ret_idx_load);
        llvm::Value *output_gep = codegen_llvm_gep(jit, output_load, output_struct_idxs);

        // get the input
        llvm::LoadInst *input_load = codegen_llvm_load(jit, user_function_arg_loader->get_user_function_input_allocs()[0], 8);

        // copy the input pointer to the output pointer
        codegen_llvm_store(jit, input_load, output_gep, 8);

        // increment ret idx
        llvm::Value *ret_idx_inc = codegen_llvm_add(jit, ret_idx_load, CodegenUtils::get_i64(1));
        codegen_llvm_store(jit, ret_idx_inc, loop->get_return_idx(), 4);

        // get the array x_dimension field of the input and add that to output_data_array_size
        llvm::Value *length = codegen_llvm_load(jit, gep_i64_i32(jit, input_load, 0, 1), 4);
        llvm::Value *inc = codegen_llvm_add(jit, length, codegen_llvm_load(jit, output_data_array_size, 4));
        codegen_llvm_store(jit, inc, output_data_array_size, 4);
    }

};

#endif //MATCHIT_FILTERBLOCK_H
