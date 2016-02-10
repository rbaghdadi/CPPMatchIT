//
// Created by Jessica Ray on 2/1/16.
//

#ifndef CPPMATCHIT_FORLOOP_H
#define CPPMATCHIT_FORLOOP_H

#include "./InstructionBlock.h"

// llvm code for a for loop
class ForLoop {

private:

    JIT *jit;
    LoopCountersIB *loop_counter_basic_block;
    ForLoopIncrementIB *for_loop_increment_basic_block;
    ForLoopConditionIB *for_loop_condition_basic_block;
    llvm::AllocaInst *max_loop_bound;
    llvm::BasicBlock *branch_to_after_counter;
    llvm::BasicBlock *branch_to_true_condition;
    llvm::BasicBlock *branch_to_false_condition;

public:

    ForLoop(JIT *jit) : jit(jit) {
        loop_counter_basic_block = new LoopCountersIB();
        for_loop_increment_basic_block = new ForLoopIncrementIB();
        for_loop_condition_basic_block = new ForLoopConditionIB();
    }

    void set_mfunction(MFunc *mfunction);

    void codegen();

    llvm::AllocaInst *get_max_loop_bound();

    llvm::BasicBlock *get_branch_to_after_counter();

    llvm::BasicBlock *get_branch_to_true_condition();

    llvm::BasicBlock *get_branch_to_false_condition();

    LoopCountersIB *get_loop_counter_basic_block();

    ForLoopIncrementIB *get_for_loop_increment_basic_block();

    ForLoopConditionIB *get_for_loop_condition_basic_block();

    void set_max_loop_bound(llvm::AllocaInst *max_loop_bound);

    void set_branch_to_after_counter(llvm::BasicBlock *branch_to_after_counter);

    void set_branch_to_true_condition(llvm::BasicBlock *branch_to_true_condition);

    void set_branch_to_false_condition(llvm::BasicBlock *branch_to_false_condition);

};

#endif //CPPMATCHIT_FORLOOP_H
