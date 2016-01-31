//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MERGESTAGES_H
#define MATCHIT_MERGESTAGES_H

#include "llvm/Transforms/Utils/Cloning.h"
#include "./Stage.h"
#include "./ImpureStage.h"

using namespace CodegenUtils;

namespace Opt {

// To merge stages, I will (rather naively) just recreate the structure of any plain old stage
// since all the stages share common structure
ImpureStage merge_stages_again(JIT *jit, Stage *stage1, Stage *stage2) {
    // inputs just to the extern call, not the whole wrapper
    std::vector<MType *> extern_input_types = stage1->get_mfunction()->get_arg_types();
    MType *merged_return_type = stage2->get_mfunction()->get_ret_type();

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

    // create the call
    extern_call_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_call_basic_block->set_extern_function(stage1->get_mfunction());
    std::vector<llvm::Value *> sliced;
    sliced.push_back(extern_init_basic_block->get_element());
    extern_call_basic_block->set_extern_args(sliced);
    extern_call_basic_block->codegen(jit);

    llvm::BasicBlock *dummy = stage.get_dummy_block();//extern_call_basic_block->get_basic_block();
    dummy->insertInto(merged_func->get_extern_wrapper());
    jit->get_builder().CreateBr(dummy);
    llvm::BasicBlock *next_call = llvm::BasicBlock::Create(llvm::getGlobalContext(), "next_call", merged_func->get_extern_wrapper());
    stage1->postprocess(&stage, dummy, next_call);//extern_call_store_basic_block->get_basic_block());
//    jit->get_builder().SetInsertPoint(dummy);
//    llvm::Instruction *last_inst;
//    for (llvm::BasicBlock::iterator iter = dummy->begin(); iter != dummy->end(); iter++) {
//        last_inst = iter;
//    }
//    last_inst->dropAllReferences();
//    last_inst->eraseFromParent();

    // create a new block for our second call
//    llvm::BasicBlock *next_call = llvm::BasicBlock::Create(llvm::getGlobalContext(), "next_call", merged_func->get_extern_wrapper());
//    jit->get_builder().CreateBr(next_call);
    jit->get_builder().SetInsertPoint(next_call);

    // the merging happens here
    // now, create the call to stage 2's function with appropriate arguments (a.k.a the result of the previous call)
    std::vector<llvm::Value *> tmp_args;
    tmp_args.push_back(extern_call_basic_block->get_data_to_return());
    stage2->get_extern_call_basic_block()->set_function(merged_func->get_extern_wrapper());
    stage2->get_extern_call_basic_block()->set_extern_args(tmp_args);
    stage2->get_extern_call_basic_block()->set_extern_function(stage2->get_mfunction());
    stage2->get_extern_call_basic_block()->codegen(jit, true);

    // TODO better way to do this? The block just shouldn't show up here
    stage2->get_extern_call_basic_block()->get_basic_block()->removeFromParent();
    stage2->postprocess(&stage, next_call, extern_call_store_basic_block->get_basic_block());
//    llvm::Instruction *last_inst;
//    for (llvm::BasicBlock::iterator iter = next_call->begin(); iter != next_call->end(); iter++) {
//        last_inst = iter;
//    }
    // last inst is a branch instruction. replace instances of dummy with store
//    last_inst->replaceUsesOfWith(dummy, extern_call_store_basic_block->get_basic_block());

    extern_call_store_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_call_store_basic_block->set_mtype(merged_return_type);
    extern_call_store_basic_block->set_data_to_store(
            stage2->get_extern_call_basic_block()->get_data_to_return());//extern_call_basic_block->get_call_result());
    extern_call_store_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
    extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
    extern_call_store_basic_block->codegen(jit);
    jit->get_builder().CreateBr(for_loop_increment_basic_block->get_basic_block());

    // return the data
    for_loop_end_basic_block->set_function(merged_func->get_extern_wrapper());
    for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
    for_loop_end_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
    for_loop_end_basic_block->codegen(jit);

    jit->dump();

    merged_func->verify_wrapper();

    return stage;
}

}

#endif // MATCHIT_MERGESTAGES