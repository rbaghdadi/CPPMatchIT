//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_STAGE_H
#define MATCHIT_STAGE_H

#include <string>
#include <tuple>
#include "llvm/IR/Function.h"
#include "./MFunc.h"
#include "./CodegenUtils.h"
#include "./InstructionBlock.h"

// TODO should probably be a struct to be easier to read, work with
// TODO please fix this soon :)
typedef std::tuple<llvm::BasicBlock *, llvm::BasicBlock *, llvm::AllocaInst *, llvm::AllocaInst *, llvm::AllocaInst *, llvm::Value *, llvm::AllocaInst *> generic_codegen_tuple;

class Stage {

protected:

    // these reference the wrapper functions, not the extern function that is what actually does work
    JIT *jit;
    MFunc *mfunction;
    std::string function_name;
    mtype_code_t input_mtype_code;
    mtype_code_t output_mtype_code;

    LoopCounterBasicBlock *loop_counter_basic_block;
    ReturnStructBasicBlock *return_struct_basic_block;
    ForLoopConditionBasicBlock *for_loop_condition_basic_block;
    ForLoopIncrementBasicBlock *for_loop_increment_basic_block;
    ForLoopEndBasicBlock *for_loop_end_basic_block;
    ExternInitBasicBlock *extern_init_basic_block;
    ExternArgPrepBasicBlock *extern_arg_prep_basic_block;
    ExternCallBasicBlock *extern_call_basic_block;
    ExternCallStoreBasicBlock *extern_call_store_basic_block;

public:

    Stage(JIT *jit, mtype_code_t input_type_code, mtype_code_t output_type_code, std::string function_name) :
            jit(jit), function_name(function_name), input_mtype_code(input_type_code), output_mtype_code(output_type_code) {
        loop_counter_basic_block = new LoopCounterBasicBlock();
        return_struct_basic_block = new ReturnStructBasicBlock();
        for_loop_condition_basic_block = new ForLoopConditionBasicBlock();
        for_loop_increment_basic_block = new ForLoopIncrementBasicBlock();
        for_loop_end_basic_block = new ForLoopEndBasicBlock();
        extern_init_basic_block = new ExternInitBasicBlock();
        extern_arg_prep_basic_block = new ExternArgPrepBasicBlock();
        extern_call_basic_block = new ExternCallBasicBlock();
        extern_call_store_basic_block = new ExternCallStoreBasicBlock();
    }

    virtual ~Stage() {
        delete loop_counter_basic_block;
        delete return_struct_basic_block;
        delete for_loop_condition_basic_block;
        delete for_loop_increment_basic_block;
        delete for_loop_end_basic_block;
        delete extern_init_basic_block;
        delete extern_arg_prep_basic_block;
        delete extern_call_basic_block;
        delete extern_call_store_basic_block;
    }

    std::string get_function_name();

    MFunc *get_mfunction();

    void set_function(MFunc *mfunction);

    virtual void codegen() = 0;

    mtype_code_t get_input_mtype_code();

    mtype_code_t get_output_mtype_code();

    LoopCounterBasicBlock *get_loop_counter_basic_block();

    ReturnStructBasicBlock *get_return_struct_basic_block();

    ForLoopConditionBasicBlock *get_for_loop_condition_basic_block();

    ForLoopIncrementBasicBlock *get_for_loop_increment_basic_block();

    ForLoopEndBasicBlock *get_for_loop_end_basic_block();

    ExternInitBasicBlock *get_extern_init_basic_block();

    ExternArgPrepBasicBlock *get_extern_arg_prep_basic_block();

    ExternCallBasicBlock *get_extern_call_basic_block();

    ExternCallStoreBasicBlock *get_extern_call_store_basic_block();

    virtual void postprocess(Stage *stage, llvm::BasicBlock *branch_from, llvm::BasicBlock *branch_into) = 0;

};

#endif //MATCHIT_STAGE_H
