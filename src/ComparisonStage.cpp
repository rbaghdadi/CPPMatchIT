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

    // Call the user function function
    call->set_user_function_arg_allocs(user_function_arg_loader->get_user_function_input_allocs());
    call->codegen(jit);
    handle_user_function_output();
    jit->get_builder().CreateBr(inner->get_increment_bb());
}

// For a comparison without outputs, the return idx is used to say how many true comparisons resulted. It's not really useful other than
// as a statistic, but I'm keeping it just to match what normally happens with a stage.
// For a comparison with outputs, the return idx is used to both say how many outputs resulted, but also pick which output element should
// be passed into the user function. If an output was passed in, but the user function returned false, then that same output should be
// passed into the next call to the user function. This keeps the actual "kept" comparisons adjacent to each other in the output list.
// Then, all the following stage has to do is pick off the first X outputs (where X is the returned value from comparison) and that stage
// will have all the comparisons that returned true.
void ComparisonStage::handle_user_function_output() {
    llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_user_function_call_result_alloc(), 1);
    llvm::Value *cmp = jit->get_builder().CreateICmpEQ(call_result_load, as_i1(0)); // compare to false
    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy",
                                                       mfunction->get_llvm_stage());
    jit->get_builder().CreateCondBr(cmp, inner->get_increment_bb(), dummy);

    // keep it and increment the return idx
    jit->get_builder().SetInsertPoint(dummy);
    llvm::LoadInst *ret_idx_load = codegen_llvm_load(jit, loop->get_return_idx(), 4);
    llvm::Value *ret_idx_inc = codegen_llvm_add(jit, ret_idx_load, Codegen::as_i32(1));
    codegen_llvm_store(jit, ret_idx_inc, loop->get_return_idx(), 4);
}
