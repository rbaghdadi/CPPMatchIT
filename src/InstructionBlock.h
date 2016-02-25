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
    bool codegen_done = false;

public:

    virtual ~InstructionBlock() { }

    llvm::BasicBlock *get_basic_block();

    void insert(llvm::Function *function);

    virtual void codegen(JIT *jit, bool no_insert = false) = 0;

};

/**
 * Give names to the arguments input to a wrapper function,
 * create AllocaInst values for them, and load them.
 * There will always be 3 inputs: the data, the number of values in all the data arrays, and the number of data structs
 */
class StageArgLoaderIB : public InstructionBlock  {

private:

    /**
     * The local allocated space for all the inputs to the stage functions.
     * Generated when codegen is called.
     */
    std::vector<llvm::AllocaInst *> args_alloc;

public:

    StageArgLoaderIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "wrapper_arg_loader");
    }

    ~StageArgLoaderIB() { }

    std::vector<llvm::AllocaInst *> get_args_alloc();

    llvm::AllocaInst *get_data();

    llvm::AllocaInst *get_data_array_size();

    llvm::AllocaInst *get_num_data_structs();

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
     * This comes from StageArgLoaderIB.
     * Required when running codegen.
     */
    llvm::AllocaInst *stage_input_arg_alloc;

    /**
     * The preallocated output space.
     * Required when running codegen.
     */
    llvm::AllocaInst *preallocated_output_space;

    /**
     * Does this extern take an output. Will be false for filter.
     * Required when running codegen.
     */
    bool has_output_param = true;

    /**
     * The loop index.
     * Required when running codegen.
     */
    llvm::AllocaInst *loop_idx_alloc;

    /**
     * Is this the SegmentationStage?
     */
    bool is_segmentation_stage = false;

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

    std::vector<llvm::AllocaInst *> get_extern_input_arg_allocs();

    llvm::AllocaInst *get_preallocated_output_space();

    void set_preallocated_output_space(llvm::AllocaInst *preallocated_output_space);

    void set_stage_input_arg_alloc(llvm::AllocaInst *stage_input_arg_alloc);

    void set_no_output_param();

    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);

    void set_segmentation_stage();

    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Create the various loop control/storage indices that will be used during execution of a stage.
 */
class ForLoopCountersIB : public InstructionBlock {

private:

    /**
     * The maximum bound on the for loop for calling the extern function.
     * This is the final input arg in the wrapper function's arg list.
     * It's here just to store it with the other counters--we don't need to restore it.
     */
    llvm::AllocaInst *loop_bound_alloc;

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
    llvm::AllocaInst *return_idx_alloc;

public:

    ~ForLoopCountersIB() { }

    ForLoopCountersIB() {
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

/**
 * Create the for loop component that increments the loop idx
 */
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
 * Create the extern call
 */
class ExternCallIB : public InstructionBlock {

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

public:

    ExternCallIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "extern.call");
    }

    ~ExternCallIB() { }

    llvm::AllocaInst *get_extern_call_result_alloc();

    void set_extern_function(llvm::Function *extern_function);

    void set_extern_arg_allocs(std::vector<llvm::AllocaInst *> extern_arg_allocs);

    void codegen(JIT *jit, bool no_insert = false);

};

// TODO can the preallocator functions be split out from MType?
class PreallocatorIB : public InstructionBlock {

protected:

    /**
     * The number of output structs being returned. Will be one-to-one for most stages
     * except for SegmentationStage since that expands the data.
     * Required when running codegen.
     */
    llvm::AllocaInst *loop_bound_alloc;

    /**
     * The fixed size of all data arrays output from this block.
     * Required when running fixed preallocator codegen.
     */
    int fixed_size = 0;

    /**
     * The type to preallocate space for.
     * Required when running codegen.
     */
    MType *base_type;

    /**
     * The number of data elements that space needs to be allocated for.
     * Required when running codegen.
     */
    llvm::Value *data_array_size;

    /**
     * The input data to the stage.
     * Required when running matched preallocator codegen.
     */
    llvm::AllocaInst *input_data;

    /**
     * If true, only allocate space for the outer struct and not the inner data elements.
     * Used when the stage is a FilterStage.
     */
    bool preallocate_outer_only = false;

    /**
     * The preallocated space.
     * Generated during codegen.
     */
    llvm::AllocaInst *preallocated_space;

public:

    void set_num_output_structs_alloc(llvm::AllocaInst *num_output_structs_alloc);

    void set_fixed_size(int fixed_size);

    void set_data_array_size(llvm::Value *data_array_size);

    void set_base_type(MType *base_type);

    void set_input_data(llvm::AllocaInst *input_data);

    void set_preallocate_outer_only(bool preallocate_outer_only);

    llvm::AllocaInst *get_preallocated_space();

    virtual void codegen(JIT *jit, bool no_insert = false) = 0;

};

class FixedPreallocatorIB : public PreallocatorIB {

public:

    FixedPreallocatorIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate", function);
    }

    void codegen(JIT *jit, bool no_insert = false);

};

class MatchedPreallocatorIB : public PreallocatorIB {

public:

    MatchedPreallocatorIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate", function);
    }

    void codegen(JIT *jit, bool no_insert = false);

};

#endif //MATCHIT_INSTRUCTIONBLOCK_H
