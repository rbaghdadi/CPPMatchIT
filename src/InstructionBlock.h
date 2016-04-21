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
 * of the stage functions when doing codegen.
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
 * Give names to the arguments input to a stage.
 * create AllocaInst values for them, and load them.
 * There will always be 3 inputs: the data, the number of values in all the data arrays, and the number of data structs
 */
class StageArgLoader : public InstructionBlock  {

private:

    /**
     * The local allocated space for all the inputs to the stage functions.
     * Generated when codegen is called.
     */
    std::vector<llvm::AllocaInst *> args_alloc;

public:

    StageArgLoader() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "stage_arg_loader");
    }

    ~StageArgLoader() { }

    std::vector<llvm::AllocaInst *> get_args_alloc();

    llvm::AllocaInst *get_data(int data_idx);

    llvm::AllocaInst *get_data_array_size();

    llvm::AllocaInst *get_num_input_elements();

    void add_arg_alloc(llvm::AllocaInst *alloc);

    void codegen(JIT *jit, bool no_insert = false);

};

/**
 * Load a single user_function_input_arg_alloc from a single input array corresponding to the current loop index.
 * This user_function_input_arg_alloc will be an input into whatever user function is being used.
 */
class UserFunctionArgLoader : public InstructionBlock {

private:

    /**
     * The allocated space for the whole argument passed into the stage function.
     * This comes from StageArgLoader.
     * Required when running codegen.
     */
    std::vector<llvm::AllocaInst *> stage_input_arg_alloc;

    /**
     * The preallocated output space.
     * Required when running codegen.
     */
    llvm::AllocaInst *preallocated_output_space;

    /**
     * Does this user function take an output. Will be false for filter.
     * Required when running codegen.
     */
    bool has_output_param = true;

    /**
     * The loop indices for the arguments pulled from the stage  unction. In the ComparisonStage,
     * there are two input streams passed into the stage, so there will be two corresponding loop_idxs.
     * Required when running codegen.
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
     * The allocated space for the input elements for the user function.
     * Generated when codegen is called.
     */
    std::vector<llvm::AllocaInst *> user_function_input_arg_alloc;

public:

    UserFunctionArgLoader() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "user_function_arg_loader");
    }

    ~UserFunctionArgLoader() { }

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

/**
 * Create the call to the user function
 */
class UserFunctionCall : public InstructionBlock {

private:

    /**
     * Arguments for this user call (generated in UserFunctionArgLoader).
     * Required when running codegen.
     */
    std::vector<llvm::AllocaInst *> user_function_arg_allocs;

    /**
     * The user function to call.
     * Required when running codegen.
     */
    llvm::Function *user_function;

    /**
     * The result of running the user function.
     * Generated when running codegen.
     */
    llvm::AllocaInst *user_function_call_result_alloc;

public:

    UserFunctionCall() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "user_function_call");
    }

    ~UserFunctionCall() { }

    llvm::AllocaInst *get_user_function_call_result_alloc();

    void set_user_function(llvm::Function *user_function);

    void set_user_function_arg_allocs(std::vector<llvm::AllocaInst *> user_function_args);

    void codegen(JIT *jit, bool no_insert = false);

};

// TODO can the preallocator functions be split out from MType?
class Preallocator : public InstructionBlock {

protected:

    /**
     * The number of output structs being returned. Will be one-to-one for most stages
     * except for SegmentationStage since that expands the data.
     * Required when running codegen.
     */
    llvm::AllocaInst *num_elements_to_alloc;

    /**
     * The type to preallocate space for.
     * Required when running codegen.
     */
    BaseField *base_field;

    /**
     * If true, only allocate space for the outer struct and not the inner data elements.
     * Used when the stage is a FilterStage.
     */
    bool preallocate_outer_only = false;

    /**
     * The preallocated space.
     * Generated during codegen. Every time codegen is called, this will be overwritten.
     */
    llvm::AllocaInst *preallocated_space;

public:

    Preallocator() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate");
    }

    void set_num_elements_to_alloc(llvm::AllocaInst *num_elements_to_alloc);

    void set_base_field(BaseField *base_field);

    llvm::AllocaInst *get_preallocated_space();

    void codegen(JIT *jit, bool no_insert = false);

};

#endif //MATCHIT_INSTRUCTIONBLOCK_H
