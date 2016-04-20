//
// Created by Jessica Ray on 3/28/16.
//

#include "./FilterStage.h"

bool FilterStage::is_filter() {
    return true;
}

void FilterStage::handle_extern_output(std::vector<llvm::AllocaInst *> preallocated_space) {
    // check if this output should be kept
    llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_extern_call_result_alloc(), 1);
    llvm::Value *cmp = jit->get_builder().CreateICmpEQ(call_result_load, as_i1(0)); // compare to false
    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy",
                                                       mfunction->get_llvm_stage());
    jit->get_builder().CreateCondBr(cmp, loop->get_increment_bb(), dummy);
    // keep it
    jit->get_builder().SetInsertPoint(dummy);
    llvm::Type *cast_to = (new MPointerType(new MPointerType((MStructType*)create_type<Element>())))->codegen_type();
    llvm::Value *output_load = jit->get_builder().CreateBitCast(codegen_llvm_load(jit, preallocated_space[0], 8), cast_to); // preallocated_space[0] has the output element space
    llvm::AllocaInst *tmp_alloc = codegen_llvm_alloca(jit, output_load->getType(), 8);
    codegen_llvm_store(jit, output_load, tmp_alloc, 8);
    stage_arg_loader->add_arg_alloc(preallocated_space[0]);
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

// input SetElements just get pass along to output setelements
std::vector<llvm::AllocaInst *> FilterStage::preallocate() {
    // The preallocated space for the outputs of this stage.
    std::vector<llvm::AllocaInst *> preallocated_space;
    preallocator->insert(mfunction->get_llvm_stage());
    jit->get_builder().CreateBr(preallocator->get_basic_block());
    jit->get_builder().SetInsertPoint(preallocator->get_basic_block());
//    llvm::AllocaInst *allocated_space = codegen_llvm_alloca(jit, llvm_int8Ptr, 8); //base_field->get_data_mtype()->codegen_type(), 8);
    llvm::Type *cast_to = (new MPointerType(new MPointerType((MStructType*)create_type<Element>())))->codegen_type();
    llvm::AllocaInst *allocated_space = codegen_llvm_alloca(jit, cast_to, 8); //base_field->get_data_mtype()->codegen_type(), 8);
    llvm::Value *space = codegen_c_malloc32(jit, codegen_llvm_mul(jit, codegen_llvm_load(jit,
                                                                                         compute_num_output_elements(), 4), as_i32(sizeof(Element*))));
    codegen_llvm_store(jit, jit->get_builder().CreateBitCast(space, cast_to), allocated_space, 8);
    preallocated_space.push_back(allocated_space);
    return preallocated_space;
}