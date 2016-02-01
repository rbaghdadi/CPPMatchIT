//
// Created by Jessica Ray on 1/31/16.
//

#include "./InstructionBlock.h"

/*
 * InstructionBlock
 */

llvm::BasicBlock *InstructionBlock::get_basic_block() {
    return bb;
}

void InstructionBlock::set_function(llvm::Function *func) {
    assert(bb);
    bb->insertInto(func);
}

/*
 * LoopCounterBasicBlock
 */

llvm::AllocaInst *LoopCounterBasicBlock::get_loop_idx() {
    return loop_idx;
}

llvm::AllocaInst *LoopCounterBasicBlock::get_loop_bound() {
    return loop_bound;
}

llvm::AllocaInst *LoopCounterBasicBlock::get_return_idx() {
    return return_idx;
}

void LoopCounterBasicBlock::set_max_bound(llvm::Value *max_bound) {
    this->max_bound = max_bound;
}

void LoopCounterBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(max_bound);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    loop_idx = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_idx(jit).get_result());
    loop_bound = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_idx(jit).get_result());
    return_idx = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_idx(jit).get_result());
    llvm::LoadInst *load = jit->get_builder().CreateLoad(max_bound);
    load->setAlignment(8);
    jit->get_builder().CreateStore(load, loop_bound)->setAlignment(8);
    codegen_done = true;
}

/*
 * ReturnStructBasicBlock
 */

llvm::AllocaInst *ReturnStructBasicBlock::get_return_struct() {
    return return_struct;
}

void ReturnStructBasicBlock::set_stage_return_type(MType *stage_return_type) {
    this->stage_return_type = stage_return_type;
}

void ReturnStructBasicBlock::set_extern_function(MFunc *extern_function) {
    this->extern_function = extern_function;
}

void ReturnStructBasicBlock::set_loop_bound(llvm::AllocaInst *loop_bound) {
    this->loop_bound = loop_bound;
}

void ReturnStructBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(stage_return_type && extern_function && loop_bound);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    return_struct = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_return_data_structure(jit, stage_return_type, *extern_function, loop_bound).get_result());
    codegen_done = true;
}

/*
 * ForLoopConditionBasicBlock
 */

llvm::Value *ForLoopConditionBasicBlock::get_loop_comparison() {
    return comparison;
}

void ForLoopConditionBasicBlock::set_loop_idx(llvm::AllocaInst *loop_idx) {
    this->loop_idx = loop_idx;
}

void ForLoopConditionBasicBlock::set_loop_bound(llvm::AllocaInst *loop_bound) {
    this->loop_bound = loop_bound;
}

// this should be the last thing called after all the optimizations and such are performed
void ForLoopConditionBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(loop_idx && loop_bound);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    comparison = CodegenUtils::create_loop_idx_condition(jit, loop_idx, loop_bound).get_result();
    codegen_done = true;
}

/*
 * ForLoopIncrementBasicBlock
 */

void ForLoopIncrementBasicBlock::set_loop_idx(llvm::AllocaInst *loop_idx) {
    this->loop_idx = loop_idx;
}

void ForLoopIncrementBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(loop_idx);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    CodegenUtils::increment_idx(jit, loop_idx);
    codegen_done = true;
}

/*
 * ForLoopEndBasicBlock
 */


void ForLoopEndBasicBlock::set_return_struct(llvm::AllocaInst *return_struct) {
    this->return_struct = return_struct;
}

void ForLoopEndBasicBlock::set_return_idx(llvm::AllocaInst *return_idx) {
    this->return_idx = return_idx;
}

void ForLoopEndBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(return_struct && return_idx);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    CodegenUtils::return_data(jit, return_struct, return_idx);
    codegen_done = true;
}

/*
 * ExternArgPrepBasicBlock
 */

std::vector<llvm::Value *> ExternArgPrepBasicBlock::get_args() {
    return args;
}

void ExternArgPrepBasicBlock::set_extern_function(MFunc *extern_function) {
    this->extern_function = extern_function;
}

void ExternArgPrepBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(extern_function);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    args = CodegenUtils::init_function_args(jit, *extern_function).get_results();
    codegen_done = true;
}

/*
 * ExternCallBasicBlock
 */

llvm::AllocaInst *ExternCallBasicBlock::get_data_to_return() {
    return data_to_return;
}

void ExternCallBasicBlock::set_extern_function(MFunc *extern_function) {
    this->extern_function = extern_function;
}

void ExternCallBasicBlock::set_extern_args(std::vector<llvm::Value *> args) {
    this->args = args;
}

// used for a stage like FilterStage which needs to store the input data rather than the output of the filter function
void ExternCallBasicBlock::override_data_to_return(llvm::AllocaInst *data_to_return) {
    this->data_to_return = data_to_return;
}

void ExternCallBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(extern_function);
    assert(!codegen_done);
    if (!no_insert) {
        jit->get_builder().SetInsertPoint(bb);
    }
    data_to_return = llvm::cast<llvm::AllocaInst>(CodegenUtils::create_extern_call(jit, *extern_function, args).get_result());
    codegen_done = true;
}

/*
 * ExternCallStoreBasicBlock
 */

void ExternCallStoreBasicBlock::set_mtype(MType *return_type) {
    this->return_type = return_type;
}

void ExternCallStoreBasicBlock::set_return_struct(llvm::AllocaInst *return_struct) {
    this->return_struct = return_struct;
}

void ExternCallStoreBasicBlock::set_return_idx(llvm::AllocaInst *return_idx) {
    this->return_idx = return_idx;
}

void ExternCallStoreBasicBlock::set_data_to_store(llvm::AllocaInst *extern_call_result) {
    this->data_to_store = extern_call_result;
}

void ExternCallStoreBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(return_type);
    assert(return_struct);
    assert(return_idx);
    assert(data_to_store);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    CodegenUtils::store_extern_result(jit, return_type, return_struct, return_idx, data_to_store);
    codegen_done = true;
}

/*
 * ExternInitBasicBlock
 */

llvm::AllocaInst *ExternInitBasicBlock::get_element() {
    return element;
}

void ExternInitBasicBlock::set_data(llvm::AllocaInst *data) {
    this->data = data;
}

void ExternInitBasicBlock::set_loop_idx(llvm::AllocaInst *loop_idx) {
    this->loop_idx = loop_idx;
}

void ExternInitBasicBlock::codegen(JIT *jit, bool no_insert) {
    assert(data);
    assert(loop_idx);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    llvm::LoadInst *loaded_data = jit->get_builder().CreateLoad(data);
    llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
    std::vector<llvm::Value *> index;
    index.push_back(cur_loop_idx);
    llvm::Value *element_gep = jit->get_builder().CreateInBoundsGEP(loaded_data, index);
    llvm::LoadInst *loaded_element = jit->get_builder().CreateLoad(element_gep);
    element = jit->get_builder().CreateAlloca(loaded_element->getType());
    jit->get_builder().CreateStore(loaded_element, element);
    codegen_done = true;
}