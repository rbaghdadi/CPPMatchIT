//
// Created by Jessica Ray on 2/1/16.
//

#include "./ForLoop.h"

llvm::BasicBlock *LoopComponent::get_basic_block() {
    return bb;
}

void LoopComponent::insert(llvm::Function *function) {
    assert(bb);
    this->function = function;
    bb->insertInto(function);
}

/*
 * LoopCountersIB
 */

llvm::AllocaInst *ForLoopCountersIB::get_loop_idx_alloc() {
    return loop_idx_alloc;
}

llvm::AllocaInst *ForLoopCountersIB::get_loop_bound_alloc() {
    return loop_bound_alloc;
}

llvm::AllocaInst *ForLoopCountersIB::get_return_idx_alloc() {
    return return_idx_alloc;
}

void ForLoopCountersIB::set_loop_bound_alloc(llvm::AllocaInst *max_loop_bound) {
    this->loop_bound_alloc = max_loop_bound;
}

void ForLoopCountersIB::codegen(JIT *jit, bool no_insert) {
    assert(loop_bound_alloc);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    loop_idx_alloc = codegen_llvm_alloca(jit, llvm_int32, 8);
    codegen_llvm_store(jit, llvm::ConstantInt::get(llvm_int32, 0), loop_idx_alloc, 8);
    return_idx_alloc = codegen_llvm_alloca(jit, llvm_int32, 8);
    codegen_llvm_store(jit, llvm::ConstantInt::get(llvm_int32, 0), return_idx_alloc, 8);
    codegen_done = true;
}

/*
 * ForLoopConditionIB
 */

llvm::Value *ForLoopConditionIB::get_loop_comparison() {
    return comparison;
}

void ForLoopConditionIB::set_loop_idx_alloc(llvm::AllocaInst *loop_idx) {
    this->loop_idx_alloc = loop_idx;
}

void ForLoopConditionIB::set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound) {
    this->max_loop_bound_alloc = max_loop_bound;
}

// this should be the last thing called after all the optimizations and such are performed
void ForLoopConditionIB::codegen(JIT *jit, bool no_insert) {
    assert(loop_idx_alloc);
    assert(max_loop_bound_alloc);
//    assert(mfunction);
    assert(!codegen_done);
    if (bb->getParent() == nullptr) {
//        bb->insertInto(mfunction->get_extern_wrapper());
    }
    jit->get_builder().SetInsertPoint(bb);
    comparison = Codegen::create_loop_condition_check(jit, loop_idx_alloc, max_loop_bound_alloc);
    codegen_done = true;
}

/*
 * ForLoopIncrementIB
 */

void ForLoopIncrementIB::set_loop_idx_alloc(llvm::AllocaInst *loop_idx) {
    this->loop_idx_alloc = loop_idx;
}

void ForLoopIncrementIB::codegen(JIT *jit, bool no_insert) {
    assert(loop_idx_alloc);
    assert(!codegen_done);
    if (!no_insert) {
        jit->get_builder().SetInsertPoint(bb);
    }
    llvm::Value *increment = codegen_llvm_add(jit, codegen_llvm_load(jit, loop_idx_alloc, 4), step_size);
    codegen_llvm_store(jit, increment, loop_idx_alloc, 4);
    codegen_done = true;
}


void ForLoop::init_codegen() {
    init_codegen(mfunction->get_extern_wrapper());
}

void ForLoop::init_codegen(llvm::Function *function) {
    counters->insert(function);
    loop_idx_increment->insert(function);
    condition->insert(function);
}

void ForLoop::codegen_loop_idx_increment() {
    loop_idx_increment->set_step_size(as_i32(1));
    loop_idx_increment->set_loop_idx_alloc(counters->get_loop_idx_alloc());
    loop_idx_increment->codegen(jit);
}

void ForLoop::codegen_return_idx_increment(llvm::Value *step_size) {
    if (step_size == nullptr) {
        return_idx_increment->set_step_size(as_i32(1));
    } else {
        return_idx_increment->set_step_size(step_size);
    }
    return_idx_increment->set_loop_idx_alloc(counters->get_return_idx_alloc());
    return_idx_increment->codegen(jit, true);
}

void ForLoop::codegen_condition() {
    condition->set_max_loop_bound_alloc(counters->get_loop_bound_alloc());
    condition->set_loop_idx_alloc(counters->get_loop_idx_alloc());
    condition->codegen(jit);
}

void ForLoop::codegen_counters(llvm::AllocaInst *loop_bound) {
    counters->set_loop_bound_alloc(loop_bound);
    counters->codegen(jit);
}

llvm::AllocaInst *ForLoop::get_loop_idx() {
    return counters->get_loop_idx_alloc();
}

llvm::AllocaInst *ForLoop::get_loop_bound() {
    return counters->get_loop_bound_alloc();
}

llvm::AllocaInst *ForLoop::get_return_idx() {
    return counters->get_return_idx_alloc();
}

llvm::BasicBlock *ForLoop::get_counter_bb() {
    return counters->get_basic_block();
}

llvm::BasicBlock *ForLoop::get_increment_bb() {
    return loop_idx_increment->get_basic_block();
}

llvm::BasicBlock *ForLoop::get_condition_bb() {
    return condition->get_basic_block();
}

ForLoopIncrementIB *ForLoop::get_increment() {
    return loop_idx_increment;
}

ForLoopConditionIB *ForLoop::get_condition() {
    return condition;
}
