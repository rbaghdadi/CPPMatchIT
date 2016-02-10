//
// Created by Jessica Ray on 2/1/16.
//

#include "./ForLoop.h"
#include "./CodegenUtils.h"

llvm::AllocaInst *ForLoop::get_max_loop_bound() {
    return max_loop_bound;
}

llvm::BasicBlock *ForLoop::get_branch_to_after_counter() {
    return branch_to_after_counter;
}

llvm::BasicBlock *ForLoop::get_branch_to_true_condition() {
    return branch_to_true_condition;
}

llvm::BasicBlock *ForLoop::get_branch_to_false_condition() {
    return branch_to_false_condition;
}

LoopCountersIB *ForLoop::get_loop_counter_basic_block() {
    return loop_counter_basic_block;
}

ForLoopIncrementIB *ForLoop::get_for_loop_increment_basic_block() {
    return for_loop_increment_basic_block;
}

ForLoopConditionIB *ForLoop::get_for_loop_condition_basic_block() {
    return for_loop_condition_basic_block;
}

void ForLoop::set_max_loop_bound(llvm::AllocaInst *max_loop_bound) {
    this->max_loop_bound = max_loop_bound;
}

void ForLoop::set_branch_to_after_counter(llvm::BasicBlock *branch_to_after_counter) {
    this->branch_to_after_counter = branch_to_after_counter;
}

void ForLoop::set_branch_to_true_condition(llvm::BasicBlock *branch_to_true_condition) {
    this->branch_to_true_condition = branch_to_true_condition;
}

void ForLoop::set_branch_to_false_condition(llvm::BasicBlock *branch_to_false_condition) {
    this->branch_to_false_condition = branch_to_false_condition;
}

void ForLoop::codegen() {
    assert(max_loop_bound);
    assert(branch_to_after_counter);
    assert(branch_to_true_condition);
    assert(branch_to_false_condition);

    loop_counter_basic_block->set_max_loop_bound_alloc(max_loop_bound);
    loop_counter_basic_block->codegen(jit, false);
    jit->get_builder().CreateBr(branch_to_after_counter);

    for_loop_condition_basic_block->set_max_loop_bound_alloc(max_loop_bound);
    for_loop_condition_basic_block->set_loop_idx_alloc(loop_counter_basic_block->get_loop_idx_alloc());
    for_loop_condition_basic_block->codegen(jit, false);
    jit->get_builder().CreateCondBr(for_loop_condition_basic_block->get_loop_comparison(), branch_to_true_condition, branch_to_false_condition);

    for_loop_increment_basic_block->set_loop_idx_alloc(loop_counter_basic_block->get_loop_idx_alloc());
    for_loop_increment_basic_block->codegen(jit, false);
    jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());
}

void ForLoop::set_mfunction(MFunc *mfunction) {
    loop_counter_basic_block->set_mfunction(mfunction);
    for_loop_increment_basic_block->set_mfunction(mfunction);
    for_loop_condition_basic_block->set_mfunction(mfunction);
}