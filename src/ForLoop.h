//
// Created by Jessica Ray on 2/1/16.
//

#ifndef CPPMATCHIT_FORLOOP_H
#define CPPMATCHIT_FORLOOP_H

#include "./InstructionBlock.h"

class ForLoop {

private:

    JIT *jit;
    ForLoopCountersIB *counters;
    ForLoopIncrementIB *loop_idx_increment;
    ForLoopIncrementIB *return_idx_increment;
    ForLoopConditionIB *condition;
    MFunc *mfunction;

public:

    ForLoop(JIT *jit, MFunc *mfunction) : jit(jit), mfunction(mfunction) {
        counters = new ForLoopCountersIB();
        loop_idx_increment = new ForLoopIncrementIB();
        return_idx_increment = new ForLoopIncrementIB();
        condition = new ForLoopConditionIB();
    }

    ForLoop(JIT *jit) : jit(jit) {
        counters = new ForLoopCountersIB();
        loop_idx_increment = new ForLoopIncrementIB();
        return_idx_increment = new ForLoopIncrementIB();
        condition = new ForLoopConditionIB();
    }

    ~ForLoop() {
        if (counters) {
            delete counters;
        }
        if (loop_idx_increment) {
            delete loop_idx_increment;
        }
        if (return_idx_increment) {
            delete return_idx_increment;
        }
        if (condition) {
            delete condition;
        }
    }

    void init_codegen();

    void init_codegen(llvm::Function *function);

    void codegen_loop_idx_increment();

    void codegen_return_idx_increment();

    void codegen_condition();

    void codegen_counters(llvm::AllocaInst *loop_bound);

    llvm::AllocaInst *get_loop_idx();

    llvm::AllocaInst *get_loop_bound();

    llvm::AllocaInst *get_return_idx();

    llvm::BasicBlock *get_counter_bb();

    llvm::BasicBlock *get_increment_bb();

    llvm::BasicBlock *get_condition_bb();

    ForLoopIncrementIB *get_increment();

    ForLoopConditionIB *get_condition();

};

#endif //CPPMATCHIT_FORLOOP_H
