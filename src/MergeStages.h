//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MERGESTAGES_H
#define MATCHIT_MERGESTAGES_H

#include "llvm/Transforms/Utils/Cloning.h"
#include "./Stage.h"
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

    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb,
                                ExternCallBasicBlock *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
        // start the merging
        // we run the first stage, then branch it to the next stage
        stage1->set_for_loop(loop);
        stage2->set_for_loop(loop);
        ExternInitBasicBlock *bridge = new ExternInitBasicBlock();
        ExternCallBasicBlock *call_bridge = new ExternCallBasicBlock();
        bridge->set_function(mfunction);
        call_bridge->set_function(mfunction);
        stage1->stage_specific_codegen(args, eibb, ecbb, bridge->get_basic_block(), loop_idx);
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

#endif // MATCHIT_MERGESTAGES