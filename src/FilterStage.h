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
            Stage(jit, "FilterStage", filter_name, param_type, param_type, MScalarType::get_bool_type()),
            filter(filter) { }

    ~FilterStage() {}

    bool is_filter() {
        return true;
    }

    void handle_extern_output(llvm::AllocaInst *output_data_array_size) {
        llvm::LoadInst *call_result_load = jit->get_builder().CreateLoad(call->get_extern_call_result_alloc());
        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(call_result_load, CodegenUtils::get_i1(0)); // compare to false
        llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy",
                                                                 mfunction->get_extern_wrapper());
        jit->get_builder().CreateCondBr(cmp, loop->get_increment_bb(), dummy);
        jit->get_builder().SetInsertPoint(dummy);
        llvm::LoadInst *output_load = jit->get_builder().CreateLoad(extern_arg_loader->get_preallocated_output_space());
        llvm::LoadInst *ret_idx_load = jit->get_builder().CreateLoad(loop->get_return_idx());
        std::vector<llvm::Value *> output_struct_idxs;
        output_struct_idxs.push_back(ret_idx_load);
        llvm::Value *output_gep = jit->get_builder().CreateInBoundsGEP(output_load, output_struct_idxs);

        // get the input
        llvm::LoadInst *input_load = jit->get_builder().CreateLoad(extern_arg_loader->get_extern_input_arg_allocs()[0]);

        // copy the input pointer to the output pointer
        jit->get_builder().CreateStore(input_load, output_gep);

        // increment ret idx
        llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(ret_idx_load, CodegenUtils::get_i64(1));
        jit->get_builder().CreateStore(ret_idx_inc, loop->get_return_idx());

        // get the array x_dimension field of the input and add that to output_data_array_size
        llvm::Value *length = jit->get_builder().CreateLoad(CodegenUtils::gep(jit, input_load, 0, 1));
        llvm::Value *inc = jit->get_builder().CreateAdd(length, jit->get_builder().CreateLoad(output_data_array_size));
        jit->get_builder().CreateStore(inc, output_data_array_size);
    }

};

#endif //MATCHIT_FILTERBLOCK_H
