//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MERGESTAGES_H
#define MATCHIT_MERGESTAGES_H

#include "llvm/Transforms/Utils/Cloning.h"
#include "./Stage.h"
#include "./ImpureStage.h"
#include "./InstructionBlock.h"

using namespace CodegenUtils;

namespace Opt {

// To merge stages, I will (rather naively) just recreate the structure of any plain old stage
// since all the stages share common structure
ImpureStage merge_stages_again(JIT *jit, Stage *stage1, Stage *stage2) {
    // inputs just to the extern call, not the whole wrapper
    std::vector<MType *> extern_input_types = stage1->get_mfunction()->get_arg_types();
    MType *merged_return_type = stage2->get_mfunction()->get_extern_wrapper_data_ret_type();//->get_ret_type();

    // create the merged function
    MFunc *merged_func = new MFunc(stage1->get_function_name() + "_" + stage2->get_function_name() + "_merged",
                                   stage1->get_mfunction()->get_associated_block() + "#" +
                                   stage2->get_mfunction()->get_associated_block(),
                                   merged_return_type, extern_input_types, jit);

    merged_func->codegen_extern_wrapper_proto();

    ImpureStage stage(stage1->get_function_name() + "_" + stage2->get_function_name() + "_merged_func",
                      jit, merged_func, stage1->get_input_mtype_code(), stage2->get_output_mtype_code());

    // now create the structure, merging the two stages in the process
    LoopCounterBasicBlock *loop_counter_basic_block = stage.get_loop_counter_basic_block();
    ReturnStructBasicBlock *return_struct_basic_block = stage.get_return_struct_basic_block();
    ForLoopConditionBasicBlock *for_loop_condition_basic_block = stage.get_for_loop_condition_basic_block();
    ForLoopIncrementBasicBlock *for_loop_increment_basic_block = stage.get_for_loop_increment_basic_block();
    ForLoopEndBasicBlock *for_loop_end_basic_block = stage.get_for_loop_end_basic_block();
    ExternInitBasicBlock *extern_init_basic_block = stage.get_extern_init_basic_block();
    ExternArgPrepBasicBlock *extern_arg_prep_basic_block = stage.get_extern_arg_prep_basic_block();
    ExternCallBasicBlock *extern_call_basic_block = stage.get_extern_call_basic_block();
    ExternCallStoreBasicBlock *extern_call_store_basic_block = stage.get_extern_call_store_basic_block();

    // initialize the function args
    extern_arg_prep_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_arg_prep_basic_block->set_extern_function(merged_func);
    extern_arg_prep_basic_block->codegen(jit);
    jit->get_builder().CreateBr(loop_counter_basic_block->get_basic_block());

    // allocate space for the loop bounds and counters
    loop_counter_basic_block->set_function(merged_func->get_extern_wrapper());
    loop_counter_basic_block->set_max_bound(extern_arg_prep_basic_block->get_args()[1]);
    loop_counter_basic_block->codegen(jit);
    jit->get_builder().CreateBr(return_struct_basic_block->get_basic_block());

    // allocate space for the return structure
    return_struct_basic_block->set_function(merged_func->get_extern_wrapper());
    return_struct_basic_block->set_extern_function(merged_func);
    return_struct_basic_block->set_loop_bound(loop_counter_basic_block->get_loop_bound());
    return_struct_basic_block->set_stage_return_type(merged_return_type);
    return_struct_basic_block->codegen(jit);
    jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());

    // loop condition
    for_loop_condition_basic_block->set_function(merged_func->get_extern_wrapper());
    for_loop_condition_basic_block->set_loop_bound(loop_counter_basic_block->get_loop_bound());
    for_loop_condition_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
    for_loop_condition_basic_block->codegen(jit);
    jit->get_builder().CreateCondBr(for_loop_condition_basic_block->get_loop_comparison(),
                                    extern_init_basic_block->get_basic_block(),
                                    for_loop_end_basic_block->get_basic_block());

    // loop index increment
    for_loop_increment_basic_block->set_function(merged_func->get_extern_wrapper());
    for_loop_increment_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
    for_loop_increment_basic_block->codegen(jit);
    jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());

    // build the body
    extern_init_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_init_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
    extern_init_basic_block->set_data(llvm::cast<llvm::AllocaInst>(extern_arg_prep_basic_block->get_args()[0]));
    extern_init_basic_block->codegen(jit);
    jit->get_builder().CreateBr(extern_call_basic_block->get_basic_block());

    // create the call to the first stage's function
    extern_call_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_call_basic_block->set_extern_function(stage1->get_mfunction());
    std::vector<llvm::Value *> call1_input_args;
    call1_input_args.push_back(extern_init_basic_block->get_element());
    extern_call_basic_block->set_extern_args(call1_input_args);
    extern_call_basic_block->codegen(jit);
    stage1->postprocess(&stage, extern_call_basic_block->get_basic_block(), stage.get_extern_call2_basic_block()->get_basic_block());

    // create the call to the second stage's function
    stage.get_extern_call2_basic_block()->set_function(merged_func->get_extern_wrapper());
    stage.get_extern_call2_basic_block()->set_extern_function(stage2->get_mfunction());
    std::vector<llvm::Value *> call2_input_args;
    call2_input_args.push_back(extern_call_basic_block->get_data_to_return());
    stage.get_extern_call2_basic_block()->set_extern_args(call2_input_args);
    stage.get_extern_call2_basic_block()->codegen(jit);
    stage.get_extern_call_basic_block()->override_data_to_return(stage.get_extern_call2_basic_block()->get_data_to_return());
    stage2->postprocess(&stage, stage.get_extern_call2_basic_block()->get_basic_block(), extern_call_store_basic_block->get_basic_block());
    stage.get_extern_call2_basic_block()->override_data_to_return(stage.get_extern_call_basic_block()->get_data_to_return());

    // store the final result
    extern_call_store_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_call_store_basic_block->set_mtype(merged_return_type);
    extern_call_store_basic_block->set_data_to_store(stage.get_extern_call2_basic_block()->get_data_to_return());
    extern_call_store_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
    extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
    extern_call_store_basic_block->codegen(jit);
    jit->get_builder().CreateBr(for_loop_increment_basic_block->get_basic_block());

    // return the data
    for_loop_end_basic_block->set_function(merged_func->get_extern_wrapper());
    for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
    for_loop_end_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
    for_loop_end_basic_block->codegen(jit);

    merged_func->verify_wrapper();

    return stage;
}

}

#endif // MATCHIT_MERGESTAGES