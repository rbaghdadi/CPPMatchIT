//
// Created by Jessica Ray on 1/31/16.
//

#ifndef MATCHIT_INSTRUCTIONBLOCK_H
#define MATCHIT_INSTRUCTIONBLOCK_H

#include "llvm/IR/BasicBlock.h"
#include "./JIT.h"
#include "./MFunc.h"
#include "./Field.h"

/*
 * Wrappers for LLVM BasicBlock types that will make up the foundation
 * of the extern wrapper functions when doing codegen_old.
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
     * Required when running codegen_old.
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
     * Generated when codegen_old is called.
     */
    std::vector<llvm::AllocaInst *> args_alloc;

    llvm::AllocaInst *secondary_alloc; // used by Filter to store its outputs

public:

    StageArgLoaderIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "wrapper_arg_loader");
    }

    ~StageArgLoaderIB() { }

    std::vector<llvm::AllocaInst *> get_args_alloc();

    llvm::AllocaInst *get_data(int data_idx);

    llvm::AllocaInst *get_data_array_size();

    llvm::AllocaInst *get_num_data_structs();

    void add_arg_alloc(llvm::AllocaInst *alloc);

    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Load a single extern_input_arg_alloc from a single input array corresponding to the current loop index.
 * This extern_input_arg_alloc will be an input into whatever extern function is being used.
 */
class UserFunctionArgLoaderIB : public InstructionBlock {

private:

    /**
     * The allocated space for the whole argument passed into the stage wrapper function.
     * This comes from StageArgLoaderIB.
     * Required when running codegen_old.
     */
    std::vector<llvm::AllocaInst *> stage_input_arg_alloc;

    /**
     * The preallocated output space.
     * Required when running codegen_old.
     */
    llvm::AllocaInst *preallocated_output_space;

    /**
     * Does this extern take an output. Will be false for filter.
     * Required when running codegen_old.
     */
    bool has_output_param = true;

    /**
     * The loop indices for the arguments pulled from the stage wrapper function. In the ComparisonStage,
     * there are two input streams passed into the wrapper, so there will be two corresponding loop_idxs.
     * Required when running codegen_old.
     */
    std::vector<llvm::AllocaInst *> loop_idx_alloc;

    /**
     * The output struct index
     */
    llvm::AllocaInst *output_idx_alloc;

    /**
     * Is this a SegmentationStage?
     */
    bool is_segmentation_stage = false;

    /**
     * Is this a FilterStage?
     */
    bool is_filter_stage = false;

    /**
     * The allocated space for the input elements for the extern function.
     * Generated when codegen_old is called.
     */
    std::vector<llvm::AllocaInst *> extern_input_arg_alloc;

public:

    UserFunctionArgLoaderIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "user_function_arg_loader");
    }

    ~UserFunctionArgLoaderIB() { }

    std::vector<llvm::AllocaInst *> get_user_function_input_allocs();

    llvm::AllocaInst *get_preallocated_output_space();

    void set_preallocated_output_space(llvm::AllocaInst *preallocated_output_space);

    void set_stage_input_arg_alloc(std::vector<llvm::AllocaInst *> stage_input_arg_alloc);

    void set_no_output_param();

    void set_output_idx_alloc(llvm::AllocaInst *output_idx_alloc);

    void set_loop_idx_alloc(std::vector<llvm::AllocaInst *> loop_idx_alloc);

    void set_segmentation_stage();

    void set_filter_stage();

    void codegen(JIT *jit, bool no_insert = false);

};
//
///**
// * Create the various loop control/storage indices that will be used during execution of a stage.
// */
//class ForLoopCountersIB : public InstructionBlock {
//
//private:
//
//    /**
//     * The maximum bound on the for loop for calling the extern function.
//     * This is the final input arg in the wrapper function's arg list.
//     * It's here just to store it with the other counters--we don't need to restore it.
//     */
//    llvm::AllocaInst *loop_bound_alloc;
//
//    /**
//     * The loop index.
//     * Generated when running codegen_old.
//     */
//    llvm::AllocaInst *loop_idx_alloc;
//
//    /**
//     * The current index input the output array for storing the results of
//     * calling the extern function.
//     * Generated when running codegen_old.
//     */
//    llvm::AllocaInst *return_idx_alloc;
//
//public:
//
//    ~ForLoopCountersIB() { }
//
//    ForLoopCountersIB() {
//        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "loop_counters");
//    }
//
//    llvm::AllocaInst *get_loop_idx_alloc();
//
//    llvm::AllocaInst *get_loop_bound_alloc();
//
//    llvm::AllocaInst *get_return_idx_alloc();
//
//    void set_loop_bound_alloc(llvm::AllocaInst *loop_bound_alloc);
//
//    void codegen_old(JIT *jit, bool no_insert = false);
//};
//
///**
// * Create the for loop component that checks whether the loop is done.
// */
//class ForLoopConditionIB : public InstructionBlock {
//
//private:
//
//    /**
//     * Current loop index.
//     * Required when running codegen_old.
//     */
//    llvm::AllocaInst *loop_idx_alloc;
//
//    /**
//     * The maximum bound on the for loop for calling the extern function.
//     * Required when running codegen_old.
//     */
//    llvm::AllocaInst *max_loop_bound_alloc;
//
//    /**
//     * The result of the condition check.
//     * Generated when running codegen_old.
//     */
//    llvm::Value *comparison;
//
//public:
//
//    ForLoopConditionIB() {
//        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.loop_condition");
//    }
//
//    ~ForLoopConditionIB() { }
//
//    llvm::Value *get_loop_comparison();
//
//    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);
//
//    void set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound_alloc);
//
//    // this should be the last thing called after all the optimizations and such are performed
//    void codegen_old(JIT *jit, bool no_insert = false);
//
//};
//
///**
// * Create the for loop component that increments the loop idx
// */
//class ForLoopIncrementIB : public InstructionBlock {
//
//private:
//
//    /**
//     * Current loop index.
//     * Required when running codegen_old.
//     */
//    llvm::AllocaInst *loop_idx_alloc;
//
//public:
//
//    ForLoopIncrementIB() {
//        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.increment");
//    }
//
//    ~ForLoopIncrementIB() { }
//
//    void set_loop_idx_alloc(llvm::AllocaInst *loop_idx_alloc);
//
//    void codegen_old(JIT *jit, bool no_insert = false);
//
//};

/**
 * Create the extern call
 */
class ExternCallIB : public InstructionBlock {

private:

    /**
     * Arguments for this extern call (generated in UserFunctionArgLoaderIB).
     * Required when running codegen_old.
     */
    std::vector<llvm::AllocaInst *> extern_arg_allocs;

    /**
     * The extern function to call.
     * Required when running codegen_old.
     */
    llvm::Function *extern_function;

    /**
     * The result of running the extern function.
     * Generated when running codegen_old.
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
     * Required when running codegen_old.
     */
    llvm::AllocaInst *loop_bound_alloc;

    /**
     * The fixed size of all data arrays output from this block.
     * Required when running fixed preallocator codegen_old.
     */
    int fixed_size = 0;

    /**
     * The type to preallocate space for.
     * Required when running codegen_old.
     */
    BaseField *base_field;

    /**
     * The number of data elements that space needs to be allocated for.
     * Required when running codegen_old.
     */
    llvm::Value *data_array_size;

    /**
     * The input data to the stage.
     * Required when running matched preallocator codegen_old.
     */
    llvm::AllocaInst *input_data;

    /**
     * If true, only allocate space for the outer struct and not the inner data elements.
     * Used when the stage is a FilterStage.
     */
    bool preallocate_outer_only = false;

    /**
     * The preallocated space.
     * Generated during codegen_old.
     */
    llvm::AllocaInst *preallocated_space;

public:

    void set_num_output_structs_alloc(llvm::AllocaInst *num_output_structs_alloc);

    void set_fixed_size(int fixed_size);

    void set_data_array_size(llvm::Value *data_array_size);

    void set_base_type(BaseField *base_type);

    void set_input_data(llvm::AllocaInst *input_data);

    void set_preallocate_outer_only(bool preallocate_outer_only);

    llvm::AllocaInst *get_preallocated_space();

    virtual void codegen(JIT *jit, bool no_insert = false) = 0;

};

class FixedPreallocatorIB : public PreallocatorIB {

public:

    FixedPreallocatorIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate");
    }

    void codegen(JIT *jit, bool no_insert = false);

};

class MatchedPreallocatorIB : public PreallocatorIB {

public:

    MatchedPreallocatorIB() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate");
    }

    void codegen(JIT *jit, bool no_insert = false);

};

#endif //MATCHIT_INSTRUCTIONBLOCK_H
