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
//    MFunc *mfunction;
    llvm::Function *func;
    LoopCounterBasicBlock *loop_counter_basic_block;
    ForLoopIncrementBasicBlock *for_loop_increment_basic_block;
    ForLoopConditionBasicBlock *for_loop_condition_basic_block;
    llvm::AllocaInst *max_loop_bound;
    llvm::BasicBlock *branch_to_after_counter;
    llvm::BasicBlock *branch_to_true_condition;
    llvm::BasicBlock *branch_to_false_condition;

public:

//    ForLoop(JIT *jit, MFunc *mfunction) : jit(jit), mfunction(mfunction) {
//        loop_counter_basic_block = new LoopCounterBasicBlock();
//        for_loop_increment_basic_block = new ForLoopIncrementBasicBlock();
//        for_loop_condition_basic_block = new ForLoopConditionBasicBlock();
//    }

    ForLoop(JIT *jit, llvm::Function *func) : jit(jit), func(func) {
        loop_counter_basic_block = new LoopCounterBasicBlock();
        for_loop_increment_basic_block = new ForLoopIncrementBasicBlock();
        for_loop_condition_basic_block = new ForLoopConditionBasicBlock();
    }

    void codegen() {
        assert(max_loop_bound);
        assert(branch_to_after_counter);
        assert(branch_to_true_condition);
        assert(branch_to_false_condition);
        assert(func);

        loop_counter_basic_block->set_function(func);
        loop_counter_basic_block->set_max_bound(max_loop_bound);
        loop_counter_basic_block->codegen(jit, false);
        jit->get_builder().CreateBr(branch_to_after_counter);

        for_loop_condition_basic_block->set_function(func);
        for_loop_condition_basic_block->set_max_bound(max_loop_bound); // TODO this is kind of repetitive with the loop counter since they both load it
        for_loop_condition_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
        for_loop_condition_basic_block->codegen(jit, false);
        jit->get_builder().CreateCondBr(for_loop_condition_basic_block->get_loop_comparison(), branch_to_true_condition, branch_to_false_condition);

        for_loop_increment_basic_block->set_function(func);
        for_loop_increment_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
        for_loop_increment_basic_block->codegen(jit, false);
        jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());
    }

    llvm::AllocaInst *get_max_loop_bound() {
        return max_loop_bound;
    }

    llvm::BasicBlock *get_branch_to_after_counter() {
        return branch_to_after_counter;
    }

    llvm::BasicBlock *get_branch_to_true_condition() {
        return branch_to_true_condition;
    }

    llvm::BasicBlock *get_branch_to_false_condition() {
        return branch_to_false_condition;
    }

    LoopCounterBasicBlock *get_loop_counter_basic_block() {
        return loop_counter_basic_block;
    }

    ForLoopIncrementBasicBlock *get_for_loop_increment_basic_block() {
        return for_loop_increment_basic_block;
    }

    ForLoopConditionBasicBlock *get_for_loop_condition_basic_block() {
        return for_loop_condition_basic_block;
    }

    void set_max_loop_bound(llvm::AllocaInst *max_loop_bound) {
        this->max_loop_bound = max_loop_bound;
    }

    void set_branch_to_after_counter(llvm::BasicBlock *branch_to_after_counter) {
        this->branch_to_after_counter = branch_to_after_counter;
    }

    void set_branch_to_true_condition(llvm::BasicBlock *branch_to_true_condition) {
        this->branch_to_true_condition = branch_to_true_condition;
    }

    void set_branch_to_false_condition(llvm::BasicBlock *branch_to_false_condition) {
        this->branch_to_false_condition = branch_to_false_condition;
    }

};

#endif //CPPMATCHIT_FORLOOP_H
