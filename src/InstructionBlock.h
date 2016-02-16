//
// Created by Jessica Ray on 1/31/16.
//

#ifndef MATCHIT_INSTRUCTIONBLOCK_H
#define MATCHIT_INSTRUCTIONBLOCK_H

#include "llvm/IR/BasicBlock.h"
#include "./JIT.h"
#include "./MFunc.h"
/*
 * Wrappers for LLVM BasicBlock types that will make up the foundation
 * of the extern wrapper functions when doing codegen.
 */

/**
 * Base block for the other foundation blocks.
 */
class InstructionBlock {
protected:

    /**
     * The BasicBlock that instructions will be inserted into
     */
    llvm::BasicBlock *bb;
    llvm::Function *function;

    /**
    * The MFunc associated with this InstructionBlock.
    * Required when running codegen.
    */
//    MFunc *mfunction;
    bool codegen_done = false;

public:

    virtual ~InstructionBlock() { }

    llvm::BasicBlock *get_basic_block();

//    MFunc *get_mfunction();

//    void set_mfunction(MFunc *mfunction);
    void insert(llvm::Function *function);

    virtual void codegen(JIT *jit, bool no_insert = false) = 0;

//    void force_insert(MFunc *mfunction);

};

/**
 * Give names to the arguments input to a wrapper function,
 * create AllocaInst values for them, and load them
 */
class WrapperArgLoaderIB : public InstructionBlock  {

private:

    /**
     * The local allocated space for all the inputs to the wrapper functions.
     * Generated when codegen is called.
     */
    std::vector<llvm::AllocaInst *> args_alloc;

public:

    WrapperArgLoaderIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "wrapper_arg_loader");
    }

    ~WrapperArgLoaderIB() { }

    std::vector<llvm::AllocaInst *> get_args_alloc();

    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Load a single extern_input_arg_alloc from a single input array corresponding to the current loop index.
 * This extern_input_arg_alloc will be an input into whatever extern function is being used.
 */
class ExternArgLoaderIB : public InstructionBlock {

private:

    /**
     * The allocated space for the whole argument passed into the wrapper function.
     * We will extract a single extern_input_arg_alloc from this.
     * This comes from InputWrapperArgLoaded.
     * Required when running codegen.
     */
    llvm::AllocaInst *wrapper_input_arg_alloc;

    /**
     * The allocated space for all the outputs.
     * Required when running codegen.
     */
    llvm::AllocaInst *preallocated_output_space;

    /**
     * The loop index.
     * Required when running codegen.
     */
    llvm::AllocaInst *loop_idx_alloc;

    /**
     * The allocated space for the input elements for the extern function.
     * Generated when codegen is called.
     */
    std::vector<llvm::AllocaInst *> extern_input_arg_alloc;

public:

    ExternArgLoaderIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "extern_arg_loader");
    }

    ~ExternArgLoaderIB() { }

    std::vector<llvm::AllocaInst *> get_extern_input_arg_alloc();

    void set_wrapper_input_arg_alloc(llvm::AllocaInst *wrapper_input_arg_alloc);

    void set_preallocated_output_space(llvm::AllocaInst *preallocated_output_space);

    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);

    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Create the various loop control/storage indices that will be used during execution of a stage.
 */
class LoopCountersIB : public InstructionBlock {

private:

    /**
     * The maximum bound on the for loop for calling the extern function.
     * This is the final input arg in the wrapper function's arg list.
     * It's here just to store it with the other counters--we don't need to restore it.
     */
    llvm::AllocaInst *max_loop_bound_alloc;

    /**
     * The loop index.
     * Generated when running codegen.
     */
    llvm::AllocaInst *loop_idx_alloc;

    /**
     * The current index input the output array for storing the results of
     * calling the extern function.
     * Generated when running codegen.
     */
    llvm::AllocaInst *output_idx_alloc;

    /**
     * The number of slots in the output array that have been allocated (malloc) so far.
     * Generated when running codegen.
     */
    llvm::AllocaInst *malloc_size_alloc;


public:

    ~LoopCountersIB() { }

    LoopCountersIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "loop_counters");
    }

    llvm::AllocaInst *get_loop_idx_alloc();

    llvm::AllocaInst *get_max_loop_bound_alloc();

    llvm::AllocaInst *get_output_idx_alloc();

    llvm::AllocaInst *get_malloc_size_alloc();

    void set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound_alloc);

    void codegen(JIT *jit, bool no_insert = false);
};

/**
 * Initializes the output struct that holds the array of output data and the
 * size of that array
 */
class WrapperOutputStructIB : public InstructionBlock  {

private:

    /**
     * The maximum bound on the for loop for calling the extern function.
     * Required when running codegen.
     */
    llvm::AllocaInst *max_loop_bound_alloc;

    /**
    * The number of slots in the output array that have been allocated (malloc) so far.
    * From LoopCountersIB.
    * Required when running codegen.
    */
    llvm::AllocaInst *malloc_size_alloc;

    /**
     * The output structure.
     * Generated when running codegen.
     */
    llvm::AllocaInst *wrapper_output_struct_alloc;


public:

    WrapperOutputStructIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "wrapper_output_struct_alloc");
    }

    ~WrapperOutputStructIB() { }

    llvm::AllocaInst *get_wrapper_output_struct_alloc();

    void set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound_alloc);

    void set_malloc_size_alloc(llvm::AllocaInst *malloc_size_alloc);

    void codegen(JIT *jit, bool no_insert = false);

};

class ForLoopConditionIB : public InstructionBlock {

private:

    /**
     * Current loop index.
     * Required when running codegen.
     */
    llvm::AllocaInst *loop_idx_alloc;

    /**
     * The maximum bound on the for loop for calling the extern function.
     * Required when running codegen.
     */
    llvm::AllocaInst *max_loop_bound_alloc;

    /**
     * The result of the condition check.
     * Generated when running codegen.
     */
    llvm::Value *comparison;

public:

    ForLoopConditionIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.loop_condition");
    }

    ~ForLoopConditionIB() { }

    llvm::Value *get_loop_comparison();

    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);

    void set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound_alloc);

    // this should be the last thing called after all the optimizations and such are performed
    void codegen(JIT *jit, bool no_insert = false);

};

class ForLoopIncrementIB : public InstructionBlock {

private:

    /**
    * Current loop index.
    * Required when running codegen.
    */
    llvm::AllocaInst *loop_idx_alloc;

public:

    ForLoopIncrementIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.increment");
    }

    ~ForLoopIncrementIB() { }

    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);

    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Finish up processing and return the generated data, whatever that may be.
 */
class ForLoopEndIB : public InstructionBlock {

private:

    /**
     * The output struct of the wrapper function
     * Required when running codegen.
     */
    llvm::AllocaInst *wrapper_output_struct_alloc;

    /**
    * The current index input the output array for storing the results of
    * calling the extern function.
    * Required when running codegen.
    */
    llvm::AllocaInst *output_idx_alloc;

public:

    ForLoopEndIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.end");
    }

    ~ForLoopEndIB() { }

    void set_wrapper_output_struct(llvm::AllocaInst *wrapper_output_struct_alloc);

    void set_output_idx_alloc(llvm::AllocaInst *output_idx_alloc);

    void codegen(JIT *jit, bool no_insert = false);

};

class ExternCallIB : public InstructionBlock  {

private:

    /**
     * Arguments for this extern call (generated in ExternArgLoaderIB).
     * Required when running codegen.
     */
    std::vector<llvm::AllocaInst *> extern_arg_allocs;

    /**
     * The extern function to call.
     * Required when running codegen.
     */
    llvm::Function *extern_function;

    /**
     * The result of running the extern function.
     * Generated when running codegen.
     */
    llvm::AllocaInst *extern_call_result_alloc;

    /**
     * If the result of the extern call is not actually going to be returned from the
     * wrapper function (as in a FilterStage), then this contains the actual data
     * that will be in the output of the wrapper.
     */
    llvm::AllocaInst *secondary_extern_call_result_alloc = nullptr;

public:

    ExternCallIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "extern.call");
    }

    ~ExternCallIB() { }

    llvm::AllocaInst *get_extern_call_result_alloc();

    llvm::AllocaInst *get_secondary_extern_call_result_alloc();

    void set_extern_function(llvm::Function *extern_function);

    void set_extern_arg_allocs(std::vector<llvm::AllocaInst *> extern_arg_allocs);

    void set_secondary_extern_call_result_alloc(llvm::AllocaInst *secondary_extern_call_result_alloc);

    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Creates the LLVM code to store the result of an extern call (or the secondary result).
 */
class ExternCallStoreIB : public InstructionBlock  {

private:

    /**
     * The output struct of the wrapper function
     * Required when running codegen.
     */
    llvm::AllocaInst *wrapper_output_struct_alloc;

    /**
     * The current index input the output array for storing the results of
     * calling the extern function.
     * Required when running codegen.
     */
    llvm::AllocaInst *output_idx_alloc;

    /**
     * The data to store.
     * Required when running codegen.
     */
    llvm::AllocaInst *data_to_store_alloc;

    /**
     * The number of slots in the output array that have been allocated (malloc) so far.
     * Required when running codegen.
     */
    llvm::AllocaInst *malloc_size_alloc;

public:

    ExternCallStoreIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store");
    }

    ~ExternCallStoreIB() { }

    void set_wrapper_output_struct_alloc(llvm::AllocaInst *wrapper_output_struct_alloc);

    void set_output_idx_alloc(llvm::AllocaInst *output_idx_alloc);

    void set_data_to_store(llvm::AllocaInst *extern_call_result);

    void set_malloc_size(llvm::AllocaInst *malloc_size);

    void codegen(JIT *jit, bool no_insert = false);

};


#endif //MATCHIT_INSTRUCTIONBLOCK_H
