//
// Created by Jessica Ray on 2/1/16.
//

#ifndef MATCHIT_FORLOOP_H
#define MATCHIT_FORLOOP_H

#include "./JIT.h"
#include "./MFunc.h"
#include "./CodegenUtils.h"

using namespace Codegen;

class LoopComponent {
protected:

    /**
     * The BasicBlock that instructions will be inserted into
     */
    llvm::BasicBlock *bb;
    llvm::Function *function;

    /**
     * The MFunc associated with this InstructionBlock.
     * Required when running codegen_old.
     */
    bool codegen_done = false;

public:

    virtual ~LoopComponent() { }

    llvm::BasicBlock *get_basic_block();

    void insert(llvm::Function *function);

    virtual void codegen(JIT *jit, bool no_insert = false) = 0;

};

/**
 * Create the various loop control/storage indices that will be used during execution of a stage.
 */
class ForLoopCounters : public LoopComponent {

private:

    /**
     * The maximum bound on the for loop for calling the extern function.
     * This is the final input arg in the wrapper function's arg list.
     * It's here just to store it with the other counters--we don't need to restore it.
     */
    llvm::AllocaInst *loop_bound_alloc;

    /**
     * The loop index.
     * Generated when running codegen_old.
     */
    llvm::AllocaInst *loop_idx_alloc;

    /**
     * The current index input the output array for storing the results of
     * calling the extern function.
     * Generated when running codegen_old.
     */
    llvm::AllocaInst *return_idx_alloc;

public:

    ~ForLoopCounters() { }

    ForLoopCounters() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "loop_counters");
    }

    llvm::AllocaInst *get_loop_idx_alloc();

    llvm::AllocaInst *get_loop_bound_alloc();

    llvm::AllocaInst *get_return_idx_alloc();

    void set_loop_bound_alloc(llvm::AllocaInst *loop_bound_alloc);

    void codegen(JIT *jit, bool no_insert = false);
};

/**
 * Create the for loop component that checks whether the loop is done.
 */
class ForLoopCondition : public LoopComponent {

private:

    /**
     * Current loop index.
     * Required when running codegen_old.
     */
    llvm::AllocaInst *loop_idx_alloc;

    /**
     * The maximum bound on the for loop for calling the extern function.
     * Required when running codegen_old.
     */
    llvm::AllocaInst *max_loop_bound_alloc;

    /**
     * The result of the condition check.
     * Generated when running codegen_old.
     */
    llvm::Value *comparison;

public:

    ForLoopCondition() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.loop_condition");
    }

    ~ForLoopCondition() { }

    llvm::Value *get_loop_comparison();

    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);

    void set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound_alloc);

    // this should be the last thing called after all the optimizations and such are performed
    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Create the for loop component that increments the loop idx
 */
class ForLoopIncrement : public LoopComponent {

private:

    /**
     * Current loop index.
     * Required when running codegen_old.
     */
    llvm::AllocaInst *loop_idx_alloc;

    llvm::Value *step_size;

public:

    ForLoopIncrement() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.increment");
    }

    ~ForLoopIncrement() { }

    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);

    void set_step_size(llvm::Value* step_size) {
        this->step_size = step_size;
    }

    void codegen(JIT *jit, bool no_insert = false);

};

class ForLoop {

private:

    JIT *jit;
    ForLoopCounters *counters; // has loop bound
    ForLoopIncrement *loop_idx_increment;
    ForLoopIncrement *return_idx_increment;
    ForLoopCondition *condition;
    MFunc *mfunction;

public:

    ForLoop(JIT *jit, MFunc *mfunction) : jit(jit), mfunction(mfunction) {
        counters = new ForLoopCounters();
        loop_idx_increment = new ForLoopIncrement();
        return_idx_increment = new ForLoopIncrement();
        condition = new ForLoopCondition();
    }

    ForLoop(JIT *jit) : jit(jit) {
        counters = new ForLoopCounters();
        loop_idx_increment = new ForLoopIncrement();
        return_idx_increment = new ForLoopIncrement();
        condition = new ForLoopCondition();
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

    void codegen_return_idx_increment(llvm::Value *step_size);

    void codegen_condition();

    void codegen_counters(llvm::AllocaInst *loop_bound);

    llvm::AllocaInst *get_loop_idx();

    llvm::AllocaInst *get_loop_bound();

    llvm::AllocaInst *get_return_idx();

    llvm::BasicBlock *get_counter_bb();

    llvm::BasicBlock *get_increment_bb();

    llvm::BasicBlock *get_condition_bb();

    ForLoopIncrement *get_increment();

    ForLoopCondition *get_condition();

};

#endif // MATCHIT_FORLOOP_H
