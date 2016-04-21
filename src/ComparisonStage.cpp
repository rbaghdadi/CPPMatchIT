//
// Created by Jessica Ray on 3/28/16.
//

#include "./ComparisonStage.h"

bool ComparisonStage::is_comparison() {
    return true;
}

std::vector<llvm::AllocaInst *> ComparisonStage::preallocate() {
    if (compareBI) { // nothing to preallocate space for
        return std::vector<llvm::AllocaInst *>();
    } else {
        return Stage::preallocate();
    }
}

llvm::AllocaInst *ComparisonStage::compute_num_output_elements() {
    llvm::Value *n2 = codegen_llvm_mul(jit, codegen_llvm_load(jit, stage_arg_loader->get_num_input_elements(), 4),
                                       codegen_llvm_load(jit, stage_arg_loader->get_num_input_elements(), 4));
    llvm::AllocaInst *n2_outputs = codegen_llvm_alloca(jit, n2->getType(), 4);
    codegen_llvm_store(jit, n2, n2_outputs, 4);
    return n2_outputs;
}

void ComparisonStage::codegen_main_loop(std::vector<llvm::AllocaInst *> preallocated_space,
                                                     llvm::BasicBlock *stage_end) {
    inner = new ForLoop(jit, mfunction);
    inner->init_codegen();
    jit->get_builder().CreateBr(inner->get_counter_bb());
    inner->codegen_counters(stage_arg_loader->get_num_input_elements());
    jit->get_builder().CreateBr(loop->get_condition_bb());
    llvm::BasicBlock *reset = llvm::BasicBlock::Create(llvm::getGlobalContext(), "reset", mfunction->get_llvm_stage());
    loop->codegen_condition();
    jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(),
                                    reset, stage_end);
    jit->get_builder().SetInsertPoint(reset);
    codegen_llvm_store(jit, as_i32(0), inner->get_loop_idx(), 4);
    jit->get_builder().CreateBr(inner->get_condition_bb());
    inner->codegen_condition();
    jit->get_builder().CreateCondBr(inner->get_condition()->get_loop_comparison(),
                                    user_function_arg_loader->get_basic_block(), loop->get_increment_bb());

    // Create the outer loop index increment
    loop->codegen_loop_idx_increment();
    jit->get_builder().CreateBr(loop->get_condition_bb());

    // Create the inner loop index incremenet
    inner->codegen_loop_idx_increment();
    jit->get_builder().CreateBr(inner->get_condition_bb());

    // Process the data through the user function in the loop body
    // Get the current input to the user function
    std::vector<llvm::AllocaInst *> loop_idxs;
    loop_idxs.push_back(get_user_function_arg_loader_idxs()[0]);
    loop_idxs.push_back(inner->get_loop_idx());

    if (!output_relation_field_types.empty()) {
        loop_idxs.push_back(loop->get_return_idx()); // there is an output
    }

    user_function_arg_loader->set_loop_idx_alloc(loop_idxs);
    user_function_arg_loader->set_stage_input_arg_alloc(get_user_function_arg_loader_data());
    user_function_arg_loader->set_output_idx_alloc(loop->get_return_idx());
    user_function_arg_loader->codegen(jit);
    jit->get_builder().CreateBr(call->get_basic_block());

    // Call the extern function
    call->set_extern_arg_allocs(user_function_arg_loader->get_user_function_input_allocs());
    call->codegen(jit);
    handle_extern_output();
    jit->get_builder().CreateBr(inner->get_increment_bb());
}

void ComparisonStage::handle_extern_output() {
    // check if this output should be kept
    llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_extern_call_result_alloc(), 1);
    llvm::Value *cmp = jit->get_builder().CreateICmpEQ(call_result_load, as_i1(0)); // compare to false
    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy",
                                                       mfunction->get_llvm_stage());
    jit->get_builder().CreateCondBr(cmp, loop->get_increment_bb(), dummy);

    // keep it
    jit->get_builder().SetInsertPoint(dummy);
    llvm::Type *cast_to = (new MPointerType(new MPointerType((MStructType*)create_type<Element>())))->codegen_type();
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
