//
// Created by Jessica Ray on 1/31/16.
//

#ifndef MATCHIT_INSTRUCTIONBLOCK_H
#define MATCHIT_INSTRUCTIONBLOCK_H

#include "llvm/IR/BasicBlock.h"
#include "./JIT.h"
#include "./CodegenUtils.h"

class InstructionBlock {
protected:

    llvm::BasicBlock *bb;
    bool codegen_done = false;
    MFunc *mfunction;

public:

    virtual ~InstructionBlock() { }

    llvm::BasicBlock *get_basic_block();

    void set_function(MFunc *func);

    MFunc *get_mfunction() {
        return mfunction;
    }

    virtual void codegen(JIT *jit, bool no_insert = false) = 0;

};

class LoopCounterBasicBlock : public InstructionBlock {

private:

    // key instructions generated in the block
    llvm::AllocaInst *loop_idx;
    llvm::AllocaInst *loop_bound;
    llvm::AllocaInst *return_idx;
    llvm::AllocaInst *malloc_size;
    // values needed for this block
    llvm::Value *max_bound;

public:

    ~LoopCounterBasicBlock() { }

    LoopCounterBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.loop_counters");
    }

    llvm::AllocaInst *get_loop_idx();

    llvm::AllocaInst *get_loop_bound();

    llvm::AllocaInst *get_return_idx();

    llvm::AllocaInst *get_malloc_size();

    void set_max_bound(llvm::Value *max_bound);

    void codegen(JIT *jit, bool no_insert = false);
};

class ReturnStructBasicBlock : public InstructionBlock  {

private:

    // key instructions generated in the block
    llvm::AllocaInst *return_struct;
    // values needed for this block
    MType *stage_return_type;
    MFunc *extern_function;
    llvm::AllocaInst *max_num_ret_elements;
    llvm::AllocaInst *malloc_size;

public:

    ReturnStructBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "return_struct");
    }

    ~ReturnStructBasicBlock() { }

    llvm::AllocaInst *get_return_struct();

    void set_stage_return_type(MType *stage_return_type);

    void set_extern_function(MFunc *extern_function);

    void set_max_num_ret_elements(llvm::AllocaInst *max_num_ret_elements);

    void set_malloc_size(llvm::AllocaInst *malloc_size);

    void codegen(JIT *jit, bool no_insert = false);

};

class ForLoopConditionBasicBlock : public InstructionBlock {

private:

    // key instructions generated in the block
    llvm::Value *comparison;
    // values needed for this block
    llvm::AllocaInst *loop_idx;
    llvm::AllocaInst *loop_bound;

public:

    ForLoopConditionBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.loop_condition");
    }

    ~ForLoopConditionBasicBlock() { }

    llvm::Value *get_loop_comparison();

    void set_loop_idx(llvm::AllocaInst *loop_idx);

    void set_max_bound(llvm::AllocaInst *loop_bound);

    // this should be the last thing called after all the optimizations and such are performed
    void codegen(JIT *jit, bool no_insert = false);

};

class ForLoopIncrementBasicBlock : public InstructionBlock {

private:

    // values needed for this block
    llvm::AllocaInst *loop_idx;

public:

    ForLoopIncrementBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.increment");
    }

    ~ForLoopIncrementBasicBlock() { }

    void set_loop_idx(llvm::AllocaInst *loop_idx);

    void codegen(JIT *jit, bool no_insert = false);

};

class ForLoopEndBasicBlock : public InstructionBlock {

private:

    // values needed for this block
    llvm::AllocaInst *return_struct;
    llvm::AllocaInst *return_idx;

public:

    ForLoopEndBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.end");
    }

    ~ForLoopEndBasicBlock() { }

    void set_return_struct(llvm::AllocaInst *return_struct);

    void set_return_idx(llvm::AllocaInst *return_idx);

    void codegen(JIT *jit, bool no_insert = false);

};

class ExternArgPrepBasicBlock : public InstructionBlock  {

private:

    // key instructions generated in the block
    std::vector<llvm::AllocaInst *> args;
    // values needed for this block
    MFunc *extern_function;

public:

    ExternArgPrepBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "arg.prep");
    }

    ~ExternArgPrepBasicBlock() { }

    std::vector<llvm::AllocaInst *> get_args();

    void set_extern_function(MFunc *extern_function);

    void codegen(JIT *jit, bool no_insert = false);

};

class ExternCallBasicBlock : public InstructionBlock  {

private:

    // key instructions generated in the block
    llvm::AllocaInst *data_to_return;
    // values needed for this block
    MFunc *extern_function;
    std::vector<llvm::Value *> args;

public:

    ExternCallBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "extern.call");
    }

    ~ExternCallBasicBlock() { }

    llvm::AllocaInst *get_data_to_return();

    void set_extern_function(MFunc *extern_function);

    void set_extern_args(std::vector<llvm::Value *> args);

    // used for a stage like FilterStage which needs to store the input data rather than the output of the filter function
    void override_data_to_return(llvm::AllocaInst *data_to_return);

    void codegen(JIT *jit, bool no_insert = false);

};

class ExternCallStoreBasicBlock : public InstructionBlock  {

private:

    // values needed for this block
    MType *return_type;
    llvm::AllocaInst *return_struct;
    llvm::AllocaInst *return_idx;
    llvm::AllocaInst *data_to_store;
    llvm::AllocaInst *malloc_size;

public:

    ExternCallStoreBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store");
    }

    ~ExternCallStoreBasicBlock() { }

    void set_mtype(MType *return_type);

    void set_return_struct(llvm::AllocaInst *return_struct);

    void set_return_idx(llvm::AllocaInst *return_idx);

    void set_data_to_store(llvm::AllocaInst *extern_call_result);

    void set_malloc_size(llvm::AllocaInst *malloc_size);

    void codegen(JIT *jit, bool no_insert = false);

};

class ExternInitBasicBlock : public InstructionBlock {

private:

    // key instructions generated in the block
    llvm::AllocaInst *element; // individual elements to be fed into function
    // values needed for this block
    llvm::AllocaInst *data; // the full data arg to pull an individual element from
    llvm::AllocaInst *loop_idx;

public:

    ExternInitBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "extern.init");
    }

    ~ExternInitBasicBlock() { }

    llvm::AllocaInst *get_element();

    void set_data(llvm::AllocaInst *data);

    void set_loop_idx(llvm::AllocaInst *loop_idx);

    void codegen(JIT *jit, bool no_insert = false);

};


#endif //MATCHIT_INSTRUCTIONBLOCK_H
