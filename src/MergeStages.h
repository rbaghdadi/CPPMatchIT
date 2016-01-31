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
                                   stage1->get_mfunction()->get_associated_block() + "#" + stage2->get_mfunction()->get_associated_block(),
                                   merged_return_type, extern_input_types, jit);

    merged_func->codegen_extern_proto();
    merged_func->codegen_extern_wrapper_proto();

    // now create the structure, merging the two stages in the process
    LoopCounterBasicBlock *loop_counter_basic_block;
    ReturnStructBasicBlock *return_struct_basic_block;
    ForLoopConditionBasicBlock *for_loop_condition_basic_block;
    ForLoopIncrementBasicBlock *for_loop_increment_basic_block;
    ForLoopEndBasicBlock *for_loop_end_basic_block;
    ExternInitBasicBlock *extern_init_basic_block;
    ExternArgPrepBasicBlock *extern_arg_prep_basic_block;
    ExternCallBasicBlock *extern_call_basic_block;
    ExternCallStoreBasicBlock *extern_call_store_basic_block;
    loop_counter_basic_block = new LoopCounterBasicBlock();
    return_struct_basic_block = new ReturnStructBasicBlock();
    for_loop_condition_basic_block = new ForLoopConditionBasicBlock();
    for_loop_increment_basic_block = new ForLoopIncrementBasicBlock();
    for_loop_end_basic_block = new ForLoopEndBasicBlock();
    extern_init_basic_block = new ExternInitBasicBlock();
    extern_arg_prep_basic_block = new ExternArgPrepBasicBlock();
    extern_call_basic_block = new ExternCallBasicBlock();
    extern_call_store_basic_block = new ExternCallStoreBasicBlock();

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
    jit->get_builder().CreateCondBr(for_loop_condition_basic_block->get_loop_comparison(), extern_init_basic_block->get_basic_block(), for_loop_end_basic_block->get_basic_block());

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
    sliced.push_back(extern_init_basic_block->get_element());//extern_arg_prep_basic_block->get_args()[0]);
    extern_call_basic_block->set_extern_args(sliced);
    extern_call_basic_block->codegen(jit);
    // TODO any postprocessing?
    // TODO make a deep copy of the extern_call_basic_block in stage 2 (need to write my own deep copy function)
    // the merging happens here
    // store a local copy of the previous call's result
    llvm::AllocaInst *tmp_alloc = jit->get_builder().CreateAlloca(extern_call_basic_block->get_call_result()->getType());
    llvm::StoreInst *tmp_store = jit->get_builder().CreateStore(extern_call_basic_block->get_call_result(), tmp_alloc);
    // now, create the call to stage 2's function with appropriate arguments (a.k.a the result of the previous call)
    std::vector<llvm::Value *> tmp_args;
    tmp_args.push_back(tmp_alloc);
    stage2->get_extern_call_basic_block()->set_function(merged_func->get_extern_wrapper());
    stage2->get_extern_call_basic_block()->set_extern_args(tmp_args);
    stage2->get_extern_call_basic_block()->set_extern_function(stage2->get_mfunction());
    stage2->get_extern_call_basic_block()->codegen(jit, true);
    // TODO better way to do this? The block just shouldn't show up here
    stage2->get_extern_call_basic_block()->get_basic_block()->removeFromParent();
    // TODO any postprocessing?
    jit->get_builder().CreateBr(extern_call_store_basic_block->get_basic_block());

    extern_call_store_basic_block->set_function(merged_func->get_extern_wrapper());
    extern_call_store_basic_block->set_mtype(merged_return_type);
    extern_call_store_basic_block->set_extern_extern_call_result(stage2->get_extern_call_basic_block()->get_call_result());//extern_call_basic_block->get_call_result());
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

    ImpureStage stage(stage1->get_function_name() + "_" + stage2->get_function_name() + "_merged_func",
                      jit, merged_func, stage1->get_input_mtype_code(), stage2->get_output_mtype_code());

    return stage;

}

void merge_blocks(JIT *jit, Stage *block1, Stage *block2) {


    // common components of both blocks
    llvm::AllocaInst *loop_idx;
    llvm::AllocaInst *ret_idx;
    llvm::AllocaInst *ret_struct;

    // clone the two functions so we don't screw up the original versions--they may need to be separated still
    llvm::ValueMap<const llvm::Value *, llvm::WeakVH> vm;
    llvm::Function *block1_function = block1->get_mfunction()->get_extern_wrapper();
    llvm::Function *block2_function = block2->get_mfunction()->get_extern_wrapper();
    llvm::Function *cloned = llvm::CloneFunction(block1_function, vm, false);
    llvm::Function *cloned_tmp = llvm::CloneFunction(block2_function, vm, false);

    // So through the clone's entry BasicBlock to pull out the common components that we need.
    // Since the BasicBlock has a specific structure, we know where to get each component from.




    // create the new MFunc to hold the merged function
    std::vector<MType *> block1_extern_arg_types = block1->get_mfunction()->get_arg_types();
    std::vector<MType *> block2_extern_arg_types = block1->get_mfunction()->get_arg_types();
    MType *extern_ret_type = block2->get_mfunction()->get_ret_type();
    // merge them

    llvm::BasicBlock *block1_extern_bb;

    // clone block1's function and remove it's store BasicBlock
    llvm::Function *cloned1 = llvm::CloneFunction(block1->get_mfunction()->get_extern_wrapper(), vm, false);
    cloned1->setName("merged_" + block1->get_function_name() + "_" + block2->get_function_name());
    jit->get_module()->getFunctionList().push_back(cloned1);
    for (llvm::Function::iterator bb_iter = cloned1->begin(); bb_iter != cloned1->end(); bb_iter++) {
        if (bb_iter->getName() == "store") {
            bb_iter->removeFromParent();
            break;
        }
    }
    // now find block1's extern BasicBlock so we can get the result from that
    llvm::Value *block1_call_res;
    for (llvm::Function::iterator bb_iter = cloned1->begin(); bb_iter != cloned1->end(); bb_iter++) {
        if (bb_iter->getName() == "for.extern_call") {
            block1_extern_bb = bb_iter;
            // go through the instructions
            for (llvm::BasicBlock::iterator inst_iter = bb_iter->begin(); inst_iter != bb_iter->end(); inst_iter++) {
                if (inst_iter->getOpcodeName() == "call") {
                    block1_call_res = inst_iter;
                } else if (inst_iter->getOpcodeName() == "br") { // comes after the call instruction
                    inst_iter->removeFromParent();
                    break;
                }
            }
            break;
        }
    }
    // TODO well, actually we need to get the result from postprocessing. All Blocks should have a postprocess method.
    // It will do something like filter out data or what not

    llvm::BasicBlock *store_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store", cloned1);

    // clone block2's function and splice it's extern call and store into the previous block
    llvm::Function *cloned2 = llvm::CloneFunction(block2->get_mfunction()->get_extern_wrapper(), vm, false);
    cloned2->setName("merged_" + block2->get_function_name() + "_" + block2->get_function_name() + "tmp");
    jit->get_module()->getFunctionList().push_back(cloned2);

    for (llvm::Function::iterator bb_iter = cloned2->begin(); bb_iter != cloned2->end(); bb_iter++) {
        if (bb_iter->getName() == "for.extern_call") {
            bb_iter->removeFromParent();
            // go through the instructions
            jit->get_builder().SetInsertPoint(block1_extern_bb);
            llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(block1_call_res->getType());
            jit->get_builder().CreateStore(block1_call_res, alloc);
            llvm::LoadInst *load;
            llvm::CallInst *call;
            for (llvm::BasicBlock::iterator inst_iter = bb_iter->begin(); inst_iter != bb_iter->end(); inst_iter++) {
                if (inst_iter->getOpcodeName() == "load") {
                    load = llvm::cast<llvm::LoadInst>(inst_iter);
                } else if (inst_iter->getOpcodeName() == "call") {
                    call = llvm::cast<llvm::CallInst>(inst_iter);
                }
            }
            load->removeFromParent();
            load->getOperandUse(0).set(alloc);
            jit->get_builder().Insert(llvm::cast<llvm::Instruction>(load));
            call->removeFromParent();
            call->getOperandUse(0).set(load);
            jit->get_builder().Insert(llvm::cast<llvm::Instruction>(call));
            jit->get_builder().CreateBr(store_bb);
            jit->get_builder().SetInsertPoint(store_bb);
            break;
        }
    }

    jit->dump();
    exit(123);
}

MFunc *merge_blocks_old(JIT *jit, Stage *block1, Stage *block2) {


    // get the components that we need out of the BasicBlocks

    // block 1
    llvm::Function *block1_function = block1->get_mfunction()->get_extern_wrapper();
//    llvm::BasicBlock *entry1;
//    llvm::BasicBlock *for_cond1;
//    llvm::BasicBlock *for_inc1;
//    llvm::BasicBlock *for_body1;
//    llvm::BasicBlock *for_extern_call1;
//    llvm::BasicBlock *for_end1;
//    llvm::BasicBlock *store_data1;
//
//    for (llvm::Function::iterator iter = block1_function->begin(); iter != block1_function->end(); iter++) {
//        std::string name = iter->getName();
//        if (name.compare("entry") == 0) {
//            entry1 = iter;
//        } else if (name.compare("for.cond") == 0) {
//            for_cond1 = iter;
//        } else if (name.compare("for.inc") == 0) {
//            for_inc1 = iter;
//        } else if (name.compare("for.body") == 0) {
//            for_body1 = iter;
//        } else if (name.compare("for.extern_call") == 0) {
//            for_extern_call1 = iter;
//        } else if (name.compare("for.end") == 0) {
//            for_end1 = iter;
//        } else if (name.compare("store") == 0) {
//            store_data1 = iter;
//        }
//    }

    // block 2
    llvm::Function *block2_function = block2->get_mfunction()->get_extern_wrapper();
//    llvm::BasicBlock *entry2;
//    llvm::BasicBlock *for_cond2;
//    llvm::BasicBlock *for_inc2;
//    llvm::BasicBlock *for_body2;
//    llvm::BasicBlock *for_extern_call2;
//    llvm::BasicBlock *for_end2;
//    llvm::BasicBlock *store_data2;
//
//    for (llvm::Function::iterator iter = block2_function->begin(); iter != block2_function->end(); iter++) {
//        std::string name = iter->getName();
//        if (name.compare("entry") == 0) {
//            entry2 = iter;
//        } else if (name.compare("for.cond") == 0) {
//            for_cond2 = iter;
//        } else if (name.compare("for.inc") == 0) {
//            for_inc2 = iter;
//        } else if (name.compare("for.body") == 0) {
//            for_body2 = iter;
//        } else if (name.compare("for.extern_call") == 0) {
//            for_extern_call2 = iter;
//        } else if (name.compare("for.end") == 0) {
//            for_end2 = iter;
//        } else if (name.compare("store") == 0) {
//            store_data2 = iter;
//        }
//    }

    // create the new MFunc to hold the merged function
    std::vector<MType *> block1_extern_arg_types = block1->get_mfunction()->get_arg_types();
    std::vector<MType *> block2_extern_arg_types = block1->get_mfunction()->get_arg_types();
    MType *extern_ret_type = block2->get_mfunction()->get_ret_type();
//
//    MFunc *func = new MFunc("merged_" + block1->get_function_name() + "_" + block2->get_function_name() , "MergedBlock", extern_ret_type, block1_extern_arg_types, jit);
//    func->codegen_extern_proto();
//    func->codegen_extern_wrapper_proto();
//
//    // merge them
//    llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", func->get_extern_wrapper());
    llvm::ValueMap<const llvm::Value *, llvm::WeakVH> vm;
    llvm::BasicBlock *block1_extern_bb;

    // clone block1's function and remove it's store BasicBlock
    llvm::Function *cloned1 = llvm::CloneFunction(block1->get_mfunction()->get_extern_wrapper(), vm, false);
    cloned1->setName("merged_" + block1->get_function_name() + "_" + block2->get_function_name());
    jit->get_module()->getFunctionList().push_back(cloned1);
    for (llvm::Function::iterator bb_iter = cloned1->begin(); bb_iter != cloned1->end(); bb_iter++) {
        if (bb_iter->getName() == "store") {
            bb_iter->removeFromParent();
            break;
        }
    }
    // now find block1's extern BasicBlock so we can get the result from that
    llvm::Value *block1_call_res;
    for (llvm::Function::iterator bb_iter = cloned1->begin(); bb_iter != cloned1->end(); bb_iter++) {
        if (bb_iter->getName() == "for.extern_call") {
            block1_extern_bb = bb_iter;
            // go through the instructions
            for (llvm::BasicBlock::iterator inst_iter = bb_iter->begin(); inst_iter != bb_iter->end(); inst_iter++) {
                if (inst_iter->getOpcodeName() == "call") {
                    block1_call_res = inst_iter;
                } else if (inst_iter->getOpcodeName() == "br") { // comes after the call instruction
                    inst_iter->removeFromParent();
                    break;
                }
            }
            break;
        }
    }
    // TODO well, actually we need to get the result from postprocessing. All Blocks should have a postprocess method.
    // It will do something like filter out data or what not

    llvm::BasicBlock *store_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store", cloned1);

    // clone block2's function and splice it's extern call and store into the previous block
    llvm::Function *cloned2 = llvm::CloneFunction(block2->get_mfunction()->get_extern_wrapper(), vm, false);
    cloned2->setName("merged_" + block2->get_function_name() + "_" + block2->get_function_name() + "tmp");
    jit->get_module()->getFunctionList().push_back(cloned2);

    for (llvm::Function::iterator bb_iter = cloned2->begin(); bb_iter != cloned2->end(); bb_iter++) {
        if (bb_iter->getName() == "for.extern_call") {
            bb_iter->removeFromParent();
            // go through the instructions
            jit->get_builder().SetInsertPoint(block1_extern_bb);
            llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(block1_call_res->getType());
            jit->get_builder().CreateStore(block1_call_res, alloc);
            llvm::LoadInst *load;
            llvm::CallInst *call;
            for (llvm::BasicBlock::iterator inst_iter = bb_iter->begin(); inst_iter != bb_iter->end(); inst_iter++) {
                if (inst_iter->getOpcodeName() == "load") {
                    load = llvm::cast<llvm::LoadInst>(inst_iter);
                } else if (inst_iter->getOpcodeName() == "call") {
                    call = llvm::cast<llvm::CallInst>(inst_iter);
                }
            }
            load->removeFromParent();
            load->getOperandUse(0).set(alloc);
            jit->get_builder().Insert(llvm::cast<llvm::Instruction>(load));
            call->removeFromParent();
            call->getOperandUse(0).set(load);
            jit->get_builder().Insert(llvm::cast<llvm::Instruction>(call));
            jit->get_builder().CreateBr(store_bb);
            jit->get_builder().SetInsertPoint(store_bb);
            break;
        }
    }

    jit->dump();
    exit(123);


//    cloned->getBasicBlockList().removeNodeFromList(store_data1);

//    jit->get_module()->getOrInsertFunction("merged_" + block1->get_function_name() + "_" + block2->get_function_name(), func->get_extern_wrapper()->getFunctionType());

//    jit->get_module()->getOrInsertFunction("merged_" + block1->get_function_name() + "_" + block2->get_function_name(), func->get_extern_wrapper()->getReturnType());
//
//    jit->dump();


//    for (llvm::BasicBlock::iterator inst_iter = entry1->begin(); inst_iter != entry1->end(); inst_iter++) {

//        unsigned int op_code = inst_iter->getOpcode();
//        std::cerr << llvm::Instruction::getOpcodeName(op_code) << std::endl;
    // get a copy of this instruction
//        llvm::Instruction *clone = inst_iter->clone();
//        clone->setName(inst_iter->getName());
//        llvm::Use *op_list = clone->getOperandList();

//    }
    return NULL;
}

// merges two blocks together so that the extern functions are immediately called one after another
MFunc *merge_blocks2(JIT *jit, Stage *block1, Stage *block2) {

    // get the components that we need out of the blocks

    // block 1
//    llvm::Function *block1_function = block1->get_mfunction()->get_extern_wrapper();
//    llvm::BasicBlock *entry1;
//    llvm::BasicBlock *for_cond1;
//    llvm::BasicBlock *for_inc1;
//    llvm::BasicBlock *for_body1;
//    llvm::BasicBlock *for_extern_call1;
//    llvm::BasicBlock *for_end1;
//    llvm::BasicBlock *store_data1;
//
//    for (llvm::Function::iterator iter = block1_function->begin(); iter != block1_function->end(); iter++) {
//        std::string name = iter->getName();
//        if (name.compare("entry") == 0) {
//            entry1 = iter;
//        } else if (name.compare("for.cond") == 0) {
//            for_cond1 = iter;
//        } else if (name.compare("for.inc") == 0) {
//            for_inc1 = iter;
//        } else if (name.compare("for.body") == 0) {
//            for_body1 = iter;
//        } else if (name.compare("for.extern_call") == 0) {
//            for_extern_call1 = iter;
//        } else if (name.compare("for.end") == 0) {
//            for_end1 = iter;
//        } else if (name.compare("store") == 0) {
//            store_data1 = iter;
//        }
//    }
//
//    // block 2
//    llvm::Function *block2_function = block2->get_mfunction()->get_extern_wrapper();
//    llvm::BasicBlock *entry2;
//    llvm::BasicBlock *for_cond2;
//    llvm::BasicBlock *for_inc2;
//    llvm::BasicBlock *for_body2;
//    llvm::BasicBlock *for_extern_call2;
//    llvm::BasicBlock *for_end2;
//    llvm::BasicBlock *store_data2;
//
//    for (llvm::Function::iterator iter = block2_function->begin(); iter != block2_function->end(); iter++) {
//        std::string name = iter->getName();
//        if (name.compare("entry") == 0) {
//            entry2 = iter;
//        } else if (name.compare("for.cond") == 0) {
//            for_cond2 = iter;
//        } else if (name.compare("for.inc") == 0) {
//            for_inc2 = iter;
//        } else if (name.compare("for.body") == 0) {
//            for_body2 = iter;
//        } else if (name.compare("for.extern_call") == 0) {
//            for_extern_call2 = iter;
//        } else if (name.compare("for.end") == 0) {
//            for_end2 = iter;
//        } else if (name.compare("store") == 0) {
//            store_data2 = iter;
//        }
//    }

    // build a new MFunc that will hold our merged function
    std::vector<MType *> block1_extern_arg_types = block1->get_mfunction()->get_arg_types();
    std::vector<MType *> block2_extern_arg_types = block1->get_mfunction()->get_arg_types();
    MType *extern_ret_type = block2->get_mfunction()->get_ret_type();

    MFunc *func = new MFunc("merged_" + block1->get_function_name() + "_" + block2->get_function_name() , "MergedBlock", extern_ret_type, block1_extern_arg_types, jit);
    func->codegen_extern_proto();
    func->codegen_extern_wrapper_proto();

    // for now, we will not unlink the BasicBlock from the individual functions because we don't know if that function
    // will be used somewhere else in the pipeline
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", func->get_extern_wrapper());
    llvm::BasicBlock *for_cond = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.cond", func->get_extern_wrapper());;
    llvm::BasicBlock *for_inc = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.inc", func->get_extern_wrapper());;
    llvm::BasicBlock *for_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.body", func->get_extern_wrapper());;
    llvm::BasicBlock *for_extern_call = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.extern_call", func->get_extern_wrapper());;
    llvm::BasicBlock *for_end = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.end", func->get_extern_wrapper());;
    llvm::BasicBlock *store_data = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store", func->get_extern_wrapper());;

    // recreate the common components of the functions
    jit->get_builder().SetInsertPoint(entry);
    // init
    FuncComp loop_idx = init_idx(jit);
    FuncComp ret_idx = init_idx(jit);
    FuncComp args = init_function_args(jit, *func);
    // return structure
    FuncComp ret_struct = init_return_data_structure(jit, extern_ret_type, *func, args.get_last());
    jit->get_builder().CreateBr(for_cond);
    // loop index
    jit->get_builder().SetInsertPoint(for_cond);
    FuncComp loop_condition = create_loop_idx_condition(jit, llvm::cast<llvm::AllocaInst>(loop_idx.get_result(0)),
                                                        llvm::cast<llvm::AllocaInst>(args.get_last()));
    jit->get_builder().CreateCondBr(loop_condition.get_result(0), for_body, for_end);
    jit->get_builder().SetInsertPoint(for_inc);
    increment_idx(jit, llvm::cast<llvm::AllocaInst>(loop_idx.get_result(0)));
    jit->get_builder().CreateBr(for_cond);
    // build the body
    jit->get_builder().SetInsertPoint(for_body);
    // get all of the data
    llvm::LoadInst *data = jit->get_builder().CreateLoad(args.get_result(0));
    llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx.get_result(0));
    // now get just the element we want to process
    std::vector<llvm::Value *> data_idx;
    data_idx.push_back(cur_loop_idx);
    llvm::Value *data_gep = jit->get_builder().CreateInBoundsGEP(data, data_idx);
    llvm::LoadInst *element = jit->get_builder().CreateLoad(data_gep);
    llvm::AllocaInst *element_alloc = jit->get_builder().CreateAlloca(element->getType());
    llvm::StoreInst *store_element =  jit->get_builder().CreateStore(element, element_alloc);
    std::vector<llvm::Value *> extern1_inputs;
    extern1_inputs.push_back(element_alloc);
    jit->get_builder().CreateBr(for_extern_call);
    jit->get_builder().SetInsertPoint(for_extern_call);
    FuncComp call_res1 = create_extern_call(jit, *(block1->get_mfunction()), extern1_inputs);
    llvm::AllocaInst *alloc_call_res1 = jit->get_builder().CreateAlloca(call_res1.get_result(0)->getType());
    jit->get_builder().CreateStore(call_res1.get_result(0), alloc_call_res1);
    // TODO what about postprocessing here?
    // llvm::Value *postprocessed_value = block1.postprocess()
    std::vector<llvm::Value *> extern2_inputs;
    extern2_inputs.push_back(alloc_call_res1);
    FuncComp call_res2 = create_extern_call(jit, *(block2->get_mfunction()), extern2_inputs);
//    llvm::Value *call_res2 = jit->get_builder().CreateCall(block2->get_mfunction()->get_extern(), extern2_inputs);

    // TODO postprocess here now
    jit->get_builder().CreateBr(store_data);
    // TODO what if this is a filter, then we may or may not run stuff
    // store the result
    jit->get_builder().SetInsertPoint(store_data);
    store_extern_result(jit, extern_ret_type, ret_struct.get_result(0), ret_idx.get_result(0), call_res2.get_result(0));
    increment_idx(jit, llvm::cast<llvm::AllocaInst>(ret_idx.get_result(0)));
    jit->get_builder().CreateBr(for_inc);

    jit->get_builder().SetInsertPoint(for_end);
    return_data(jit, ret_struct.get_result(0), ret_idx.get_result(0));

    return func;

}

}


#endif //MATCHIT_MERGESTAGES_H
