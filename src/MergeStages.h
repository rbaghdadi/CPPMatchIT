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



class Merge : public Stage {

private:

    // TODO these really need to be deep copied instead
    Stage *stage1;
    Stage *stage2;

public:

    Merge(JIT *jit, Stage *stage1, Stage *stage2) : Stage(jit, stage1->get_input_mtype_code(), stage2->get_output_mtype_code(),
                                                          stage1->get_function_name() + "_" + stage2->get_function_name()),
                                                    stage1(stage1), stage2(stage2) {
        MFunc *func = new MFunc(function_name, "MergeStage", stage2->get_mfunction()->get_extern_wrapper_data_ret_type(),
                                stage1->get_mfunction()->get_arg_types(), jit);
        set_function(func);
        func->codegen_extern_proto();
        func->codegen_extern_wrapper_proto();
    }

//    void base_codegen() {
//        // initialize the function args
//        extern_arg_prep_basic_block->set_function(mfunction);
//        extern_arg_prep_basic_block->set_extern_function(mfunction);
//        extern_arg_prep_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(loop->get_loop_counter_basic_block()->get_basic_block());
//
//        // loop components
//        loop->set_max_loop_bound(llvm::cast<llvm::AllocaInst>(last(extern_arg_prep_basic_block->get_args())));
//        loop->set_branch_to_after_counter(return_struct_basic_block->get_basic_block());
//        loop->set_branch_to_true_condition(extern_init_basic_block->get_basic_block());
//        loop->set_branch_to_false_condition(for_loop_end_basic_block->get_basic_block());
//        loop->codegen();
//
//        // allocate space for the return structure
//        return_struct_basic_block->set_function(mfunction);
//        return_struct_basic_block->set_extern_function(mfunction);
//        if (stage1->get_mfunction()->get_associated_block() == "ComparisonBlock" || stage2->get_mfunction()->get_associated_block() == "ComparisonBlock") {
//            // output size is N^2 where N is the loop bound
//            llvm::Value *mult = jit->get_builder().CreateMul(loop->get_loop_counter_basic_block()->get_loop_bound(),
//                                                             loop->get_loop_counter_basic_block()->get_loop_bound());
//            llvm::AllocaInst *mult_alloc = jit->get_builder().CreateAlloca(mult->getType());
//            jit->get_builder().CreateStore(mult, mult_alloc)->setAlignment(8);
//            return_struct_basic_block->set_max_num_ret_elements(mult_alloc);
//        } else {
//            return_struct_basic_block->set_max_num_ret_elements(loop->get_loop_counter_basic_block()->get_loop_bound());
//        }
//        return_struct_basic_block->set_stage_return_type(mfunction->get_extern_wrapper_data_ret_type());
//        return_struct_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(loop->get_for_loop_condition_basic_block()->get_basic_block());
//
////        // start the merging
////        // we run the first stage, then branch it to the next stage
////        stage1->set_for_loop(loop);
////        stage2->set_for_loop(loop);
////        ExternInitBasicBlock *bridge = new ExternInitBasicBlock();
////        ExternCallBasicBlock *call_bridge = new ExternCallBasicBlock();
////        extern_init_basic_block->set_function(mfunction);
////        extern_call_basic_block->set_function(mfunction);
////        bridge->set_function(mfunction);
////        call_bridge->set_function(mfunction);
////        stage1->stage_specific_codegen(extern_arg_prep_basic_block->get_args(), extern_init_basic_block,
////                                       extern_call_basic_block, bridge->get_basic_block(), loop->get_loop_counter_basic_block()->get_loop_idx());//extern_call_store_basic_block->get_basic_block());
////        jit->get_builder().SetInsertPoint(bridge->get_basic_block());
////
////        // TODO if the second stage is a comparison, we need to split the data in this. Having a splitter is probably a good idea
////        // store the data to be passed along as if it were normal data
////        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(extern_call_basic_block->get_data_to_return()->getType(), 0);
////        llvm::LoadInst *load = jit->get_builder().CreateLoad(extern_call_basic_block->get_data_to_return());
////        std::vector<llvm::Value *> gep_idxs;
////        gep_idxs.push_back(llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(llvm::getGlobalContext()),0));
////        llvm::Value *gep = jit->get_builder().CreateInBoundsGEP(alloc, gep_idxs);
////        jit->get_builder().CreateStore(load, jit->get_builder().CreateLoad(gep));
////        std::vector<llvm::AllocaInst *> inputs;
////        inputs.push_back(alloc);//extern_call_basic_block->get_data_to_return());
////        llvm::AllocaInst *zero_alloc = jit->get_builder().CreateAlloca(llvm::IntegerType::getInt64Ty(llvm::getGlobalContext()));
////        jit->get_builder().CreateStore(llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(llvm::getGlobalContext()),0), zero_alloc)->setAlignment(8);
////        stage2->stage_specific_codegen(inputs, bridge, call_bridge, extern_call_store_basic_block->get_basic_block(),
////                                       zero_alloc);
//        stage_specific_codegen(extern_arg_prep_basic_block->get_args(), extern_init_basic_block, extern_call_basic_block,
//                               extern_call_store_basic_block->get_basic_block(),
//                               loop->get_loop_counter_basic_block()->get_loop_idx());
//
//        // store the result
//        extern_call_store_basic_block->set_function(mfunction);
//        extern_call_store_basic_block->set_mtype(mfunction->get_extern_wrapper_data_ret_type());
////        extern_call_store_basic_block->set_data_to_store(call_bridge->get_data_to_return());
//        extern_call_store_basic_block->set_data_to_store(extern_call_basic_block->get_data_to_return());
//        extern_call_store_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
//        extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
//        extern_call_store_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(loop->get_for_loop_increment_basic_block()->get_basic_block());
//
//        // return the data
//        for_loop_end_basic_block->set_function(mfunction);
//        for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
//        for_loop_end_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
//        for_loop_end_basic_block->codegen(jit);
//
//        // cleanup
////        delete bridge;
////        delete call_bridge;
//
//        mfunction->verify_wrapper();
//
//    }

    // TODO remove base codegen and put the specifics in here
    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb,
                                ExternCallBasicBlock *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
        // start the merging
        // we run the first stage, then branch it to the next stage
        stage1->set_for_loop(loop);
        stage2->set_for_loop(loop);
        ExternInitBasicBlock *bridge = new ExternInitBasicBlock();
        ExternCallBasicBlock *call_bridge = new ExternCallBasicBlock();
//        eibb->set_function(mfunction);
//        ecbb->set_function(mfunction);
        bridge->set_function(mfunction);
        call_bridge->set_function(mfunction);
        stage1->stage_specific_codegen(args, eibb,
                                       ecbb, bridge->get_basic_block(), loop_idx);//extern_call_store_basic_block->get_basic_block());
        jit->get_builder().SetInsertPoint(bridge->get_basic_block());

        // TODO if the second stage is a comparison, we need to split the data in this. Having a splitter is probably a good idea
        // store the data to be passed along as if it were normal data
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(ecbb->get_data_to_return()->getType(), 0);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(ecbb->get_data_to_return());
        std::vector<llvm::Value *> gep_idxs;
        gep_idxs.push_back(llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(llvm::getGlobalContext()),0));
        llvm::Value *gep = jit->get_builder().CreateInBoundsGEP(alloc, gep_idxs);
        jit->get_builder().CreateStore(load, jit->get_builder().CreateLoad(gep));
        std::vector<llvm::AllocaInst *> inputs;
        inputs.push_back(alloc);
        llvm::AllocaInst *zero_alloc = jit->get_builder().CreateAlloca(llvm::IntegerType::getInt64Ty(llvm::getGlobalContext()));
        jit->get_builder().CreateStore(llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(llvm::getGlobalContext()),0), zero_alloc)->setAlignment(8);
        stage2->stage_specific_codegen(inputs, bridge, call_bridge, branch_to, zero_alloc);
        ecbb->override_data_to_return(call_bridge->get_data_to_return());

        delete bridge;
        delete call_bridge;
    }

};




/*
// To merge stages, I will (rather naively) just recreate the structure of any plain old stage
// since all the stages share common structure
ImpureStage *merge_stages_again(JIT *jit, Stage *stage1, Stage *stage2) {
    // inputs just to the extern call, not the whole wrapper
    std::vector<MType *> extern_input_types = stage1->get_mfunction()->get_arg_types();
    MType *merged_return_type = stage2->get_mfunction()->get_extern_wrapper_data_ret_type();//->get_ret_type();

    // create the merged function
    MFunc *merged_func = new MFunc(stage1->get_function_name() + "_" + stage2->get_function_name() + "_merged",
                                   stage1->get_mfunction()->get_associated_block() + "#" +
                                   stage2->get_mfunction()->get_associated_block(),
                                   merged_return_type, extern_input_types, jit);

    merged_func->codegen_extern_wrapper_proto();

    ImpureStage *stage = new ImpureStage(stage1->get_function_name() + "_" + stage2->get_function_name() + "_merged_func",
                                         jit, merged_func, stage1->get_input_mtype_code(), stage2->get_output_mtype_code());

    // now create the structure, merging the two stages in the process
    LoopCounterBasicBlock *loop_counter_basic_block = stage->get_loop_counter_basic_block();
    ReturnStructBasicBlock *return_struct_basic_block = stage->get_return_struct_basic_block();
    ForLoopConditionBasicBlock *for_loop_condition_basic_block = stage->get_for_loop_condition_basic_block();
    ForLoopIncrementBasicBlock *for_loop_increment_basic_block = stage->get_for_loop_increment_basic_block();
    ForLoopEndBasicBlock *for_loop_end_basic_block = stage->get_for_loop_end_basic_block();
    ExternInitBasicBlock *extern_init_basic_block = stage->get_extern_init_basic_block();
    ExternArgPrepBasicBlock *extern_arg_prep_basic_block = stage->get_extern_arg_prep_basic_block();
    ExternCallBasicBlock *extern_call_basic_block = stage->get_extern_call_basic_block();
    ExternCallStoreBasicBlock *extern_call_store_basic_block = stage->get_extern_call_store_basic_block();

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
    for_loop_condition_basic_block->set_max_bound(loop_counter_basic_block->get_loop_bound());
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
    stage1->postprocess(stage, extern_call_basic_block->get_basic_block(), stage->get_extern_call2_basic_block()->get_basic_block());

    // create the call to the second stage's function
    stage->get_extern_call2_basic_block()->set_function(merged_func->get_extern_wrapper());
    stage->get_extern_call2_basic_block()->set_extern_function(stage2->get_mfunction());
    std::vector<llvm::Value *> call2_input_args;
    call2_input_args.push_back(extern_call_basic_block->get_data_to_return());
    stage->get_extern_call2_basic_block()->set_extern_args(call2_input_args);
    stage->get_extern_call2_basic_block()->codegen(jit);
    stage->get_extern_call_basic_block()->override_data_to_return(stage->get_extern_call2_basic_block()->get_data_to_return());
    stage2->postprocess(stage, stage->get_extern_call2_basic_block()->get_basic_block(), extern_call_store_basic_block->get_basic_block());
    stage->get_extern_call2_basic_block()->override_data_to_return(stage->get_extern_call_basic_block()->get_data_to_return());

    // store the final result
    extern_call_store_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_call_store_basic_block->set_mtype(merged_return_type);
    extern_call_store_basic_block->set_data_to_store(stage->get_extern_call2_basic_block()->get_data_to_return());
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
*/

#endif // MATCHIT_MERGESTAGES