//
// Created by Jessica Ray on 1/28/16.
//

#include "./Stage.h"

std::string Stage::get_function_name() {
    return function_name;
}

MFunc *Stage::get_mfunction() {
    return mfunction;
}

void Stage::set_function(MFunc *mfunction) {
    loop = new ForLoop(jit, mfunction);
    this->mfunction = mfunction;
}

void Stage::set_for_loop(ForLoop *loop) {
    this->loop = loop;
}

mtype_code_t Stage::get_input_mtype_code() {
    return input_mtype_code;
}

mtype_code_t Stage::get_output_mtype_code() {
    return output_mtype_code;
}

ReturnStructBasicBlock *Stage::get_return_struct_basic_block() {
    return return_struct_basic_block;
}

ForLoopEndBasicBlock *Stage::get_for_loop_end_basic_block() {
    return for_loop_end_basic_block;
}

ExternInitBasicBlock *Stage::get_extern_init_basic_block() {
    return extern_init_basic_block;
}

ExternArgPrepBasicBlock *Stage::get_extern_arg_prep_basic_block() {
    return extern_arg_prep_basic_block;
}

ExternCallBasicBlock *Stage::get_extern_call_basic_block() {
    return extern_call_basic_block;
}

ExternCallStoreBasicBlock *Stage::get_extern_call_store_basic_block() {
    return extern_call_store_basic_block;
}

llvm::BasicBlock *Stage::branch_to_after_store() {
    return loop->get_for_loop_increment_basic_block()->get_basic_block();
}


void Stage::base_codegen() {
    // initialize the function args
    extern_arg_prep_basic_block->set_function(mfunction);
    extern_arg_prep_basic_block->set_extern_function(mfunction);
    extern_arg_prep_basic_block->codegen(jit, false);
    jit->get_builder().CreateBr(loop->get_loop_counter_basic_block()->get_basic_block());

    // loop components
    loop->set_max_loop_bound(llvm::cast<llvm::AllocaInst>(last(extern_arg_prep_basic_block->get_args())));
    loop->set_branch_to_after_counter(return_struct_basic_block->get_basic_block());
    loop->set_branch_to_true_condition(extern_init_basic_block->get_basic_block());
    loop->set_branch_to_false_condition(for_loop_end_basic_block->get_basic_block());
    loop->codegen();

    // allocate space for the return structure
    return_struct_basic_block->set_function(mfunction);
    return_struct_basic_block->set_extern_function(mfunction);
    if (mfunction->get_associated_block() == "ComparisonStage") {
        jit->get_builder().SetInsertPoint(return_struct_basic_block->get_basic_block());
        // output size is N^2 where N is the loop bound
        llvm::LoadInst *loop_bound = jit->get_builder().CreateLoad(loop->get_loop_counter_basic_block()->get_loop_bound());
        llvm::Value *mult = jit->get_builder().CreateMul(loop_bound, loop_bound);
        llvm::AllocaInst *mult_alloc = jit->get_builder().CreateAlloca(mult->getType());
        jit->get_builder().CreateStore(mult, mult_alloc)->setAlignment(8);
        return_struct_basic_block->set_max_num_ret_elements(mult_alloc);
    } else {
        return_struct_basic_block->set_max_num_ret_elements(loop->get_loop_counter_basic_block()->get_loop_bound());
    }
    return_struct_basic_block->set_stage_return_type(mfunction->get_extern_wrapper_data_ret_type());
    return_struct_basic_block->codegen(jit, false);
    jit->get_builder().CreateBr(loop->get_for_loop_condition_basic_block()->get_basic_block());

    // codegen the body for the appropriate stage
    extern_init_basic_block->set_function(mfunction);
    extern_call_basic_block->set_function(mfunction);
    stage_specific_codegen(extern_arg_prep_basic_block->get_args(), extern_init_basic_block, extern_call_basic_block,
                           extern_call_store_basic_block->get_basic_block(), loop->get_loop_counter_basic_block()->get_loop_idx());

    // store the result
    extern_call_store_basic_block->set_function(mfunction);
    extern_call_store_basic_block->set_mtype(mfunction->get_extern_wrapper_data_ret_type());
    extern_call_store_basic_block->set_data_to_store(extern_call_basic_block->get_data_to_return());
    extern_call_store_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
    extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
    extern_call_store_basic_block->codegen(jit, false);
    jit->get_builder().CreateBr(branch_to_after_store());

    // return the data
    for_loop_end_basic_block->set_function(mfunction);
    for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
    for_loop_end_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
    for_loop_end_basic_block->codegen(jit, false);

    mfunction->verify_wrapper();
}