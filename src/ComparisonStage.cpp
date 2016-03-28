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
        assert(false); // can't do the other two yet (will need to allocate n^2 setelement data though)
    }
}

llvm::CallInst *ComparisonStage::codegen_main_loop(std::vector<llvm::AllocaInst *> preallocated_space,
                                                   llvm::BasicBlock *stage_end) {
    ForLoop inner(jit, mfunction);
    inner.init_codegen();
    jit->get_builder().CreateBr(inner.get_counter_bb());
    inner.codegen_counters(stage_arg_loader->get_num_data_structs());
    jit->get_builder().CreateBr(loop->get_condition_bb());
    llvm::BasicBlock *reset = llvm::BasicBlock::Create(llvm::getGlobalContext(), "reset", mfunction->get_extern_wrapper());
    loop->codegen_condition();
    jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(),
                                    reset, stage_end);
    jit->get_builder().SetInsertPoint(reset);
    codegen_llvm_store(jit, as_i32(0), inner.get_loop_idx(), 4);
    jit->get_builder().CreateBr(inner.get_condition_bb());
    inner.codegen_condition();
    jit->get_builder().CreateCondBr(inner.get_condition()->get_loop_comparison(),
                                    user_function_arg_loader->get_basic_block(), loop->get_increment_bb());

    // Create the outer loop index increment
    loop->codegen_loop_idx_increment();
    jit->get_builder().CreateBr(loop->get_condition_bb());

    // Create the inner loop index incremenet
    inner.codegen_loop_idx_increment();
    jit->get_builder().CreateBr(inner.get_condition_bb());

    // Process the data through the user function in the loop body
    // Get the current input to the user function
    std::vector<llvm::AllocaInst *> loop_idxs;
    loop_idxs.push_back(get_user_function_arg_loader_idxs()[0]);
    loop_idxs.push_back(inner.get_loop_idx());
    user_function_arg_loader->set_loop_idx_alloc(loop_idxs);
    user_function_arg_loader->set_stage_input_arg_alloc(get_user_function_arg_loader_data());
    user_function_arg_loader->set_output_idx_alloc(loop->get_return_idx());
    user_function_arg_loader->codegen(jit);
    jit->get_builder().CreateBr(call->get_basic_block());

    // Call the extern function
    call->set_extern_arg_allocs(user_function_arg_loader->get_user_function_input_allocs());
    call->codegen(jit);
    handle_extern_output(preallocated_space); // doesn't really do anything for now since I'm not returning anything
    jit->get_builder().CreateBr(inner.get_increment_bb());

}

llvm::AllocaInst *ComparisonStage::finish_stage(unsigned int fixed_size) {
    return nullptr;
}