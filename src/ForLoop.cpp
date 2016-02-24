//
// Created by Jessica Ray on 2/1/16.
//

#include "./ForLoop.h"
#include "./CodegenUtils.h"

void ForLoop::init_codegen() {
    init_codegen(mfunction->get_extern_wrapper());
}

void ForLoop::init_codegen(llvm::Function *function) {
    counters->insert(function);
    loop_idx_increment->insert(function);
    condition->insert(function);
}

void ForLoop::codegen_loop_idx_increment() {
    loop_idx_increment->set_loop_idx_alloc(counters->get_loop_idx_alloc());
    loop_idx_increment->codegen(jit);
}

void ForLoop::codegen_return_idx_increment() {
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
