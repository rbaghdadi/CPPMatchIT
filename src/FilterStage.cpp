//
// Created by Jessica Ray on 3/28/16.
//

#include "./FilterStage.h"

bool FilterStage::is_filter() {
    return true;
}

// pass along Element if should be kept, otherwise, don't do anything with it
void FilterStage::handle_user_function_output() {
    // check if this output should be kept
    llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_user_function_call_result_alloc(), 1);
    llvm::Value *cmp = jit->get_builder().CreateICmpEQ(call_result_load, as_i1(0)); // compare to false
    llvm::BasicBlock *keep = llvm::BasicBlock::Create(llvm::getGlobalContext(), "keep_element",
                                                       mfunction->get_llvm_stage());
    llvm::BasicBlock *remove = llvm::BasicBlock::Create(llvm::getGlobalContext(), "remove_element", mfunction->get_llvm_stage());
    jit->get_builder().CreateCondBr(cmp, remove, keep);

    // remove it
    jit->get_builder().SetInsertPoint(remove);
    codegen_mdelete(jit, codegen_llvm_load(jit, user_function_arg_loader->get_user_function_input_allocs()[0], 8));
    jit->get_builder().CreateBr(loop->get_increment_bb());

    // keep it
    jit->get_builder().SetInsertPoint(keep);
    MStructType *element = (MStructType*)create_type<Element>();
    MPointerType ptr(element);
    MPointerType ptrptr(&ptr);
    llvm::Type *cast_to = ptrptr.codegen_type();
    llvm::Value *output_load = jit->get_builder().CreateBitCast(codegen_llvm_load(jit, stage_arg_loader->get_args_alloc()[2]/*[2] is the output Element param*/, 8), cast_to);
    llvm::AllocaInst *tmp_alloc = codegen_llvm_alloca(jit, output_load->getType(), 8);
    codegen_llvm_store(jit, output_load, tmp_alloc, 8);
    llvm::LoadInst *ret_idx_load = codegen_llvm_load(jit, loop->get_return_idx(), 4);
    std::vector<llvm::Value *> output_struct_idxs;
    output_struct_idxs.push_back(ret_idx_load);
    llvm::Value *output_gep = codegen_llvm_gep(jit, output_load, output_struct_idxs);

    // get the input
    llvm::LoadInst *input_load = codegen_llvm_load(jit, user_function_arg_loader->get_user_function_input_allocs()[0], 8);

    // copy the input pointer to the output pointer
    codegen_llvm_store(jit, input_load, output_gep, 8);

    // increment return idx
    llvm::Value *ret_idx_inc = codegen_llvm_add(jit, ret_idx_load, Codegen::as_i32(1));
    codegen_llvm_store(jit, ret_idx_inc, loop->get_return_idx(), 4);
}

std::vector<llvm::AllocaInst *> FilterStage::preallocate() { // there isn't any new space to allocate for this
    std::vector<llvm::AllocaInst *> preallocated_space;
    return std::vector<llvm::AllocaInst *>();//preallocated_space;
}

llvm::AllocaInst *FilterStage::finish_stage() {
////     Clear out the space allocated for output Element objects that wasn't used because of the input Element objects that were dropped.
//    llvm::Value *realloc_size = codegen_llvm_mul(jit, codegen_llvm_load(jit, loop->get_return_idx(), 4), as_i32(sizeof(Element *)));
//    llvm::LoadInst *loaded_arg = codegen_llvm_load(jit, stage_arg_loader->get_args_alloc()[2], 8);
//    llvm::Value *reallocated = codegen_mrealloc_and_cast(jit, loaded_arg, realloc_size, loaded_arg->getType()); // shrink size
//    codegen_llvm_store(jit, reallocated, stage_arg_loader->get_args_alloc()[2], 8); // overrwrite the current pointer that was there
    return Stage::finish_stage();
}