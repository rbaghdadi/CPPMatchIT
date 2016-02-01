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
#include "./ForLoop.h"
#include "./Utils.h"

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
    ForLoop *loop;


//    LoopCounterBasicBlock *loop_counter_basic_block;
    ReturnStructBasicBlock *return_struct_basic_block;
//    ForLoopConditionBasicBlock *for_loop_condition_basic_block;
//    ForLoopIncrementBasicBlock *for_loop_increment_basic_block;
    ForLoopEndBasicBlock *for_loop_end_basic_block;
    ExternInitBasicBlock *extern_init_basic_block;
    ExternArgPrepBasicBlock *extern_arg_prep_basic_block;
    ExternCallBasicBlock *extern_call_basic_block;
    ExternCallStoreBasicBlock *extern_call_store_basic_block;

public:

    Stage(JIT *jit, mtype_code_t input_type_code, mtype_code_t output_type_code, std::string function_name) :
            jit(jit), function_name(function_name), input_mtype_code(input_type_code), output_mtype_code(output_type_code) {
//        loop_counter_basic_block = new LoopCounterBasicBlock();
        return_struct_basic_block = new ReturnStructBasicBlock();
//        for_loop_condition_basic_block = new ForLoopConditionBasicBlock();
//        for_loop_increment_basic_block = new ForLoopIncrementBasicBlock();
        for_loop_end_basic_block = new ForLoopEndBasicBlock();
        extern_init_basic_block = new ExternInitBasicBlock();
        extern_arg_prep_basic_block = new ExternArgPrepBasicBlock();
        extern_call_basic_block = new ExternCallBasicBlock();
        extern_call_store_basic_block = new ExternCallStoreBasicBlock();
    }

    virtual ~Stage() {
//        delete loop_counter_basic_block;
        delete return_struct_basic_block;
//        delete for_loop_condition_basic_block;
//        delete for_loop_increment_basic_block;
        delete for_loop_end_basic_block;
        delete extern_init_basic_block;
        delete extern_arg_prep_basic_block;
        delete extern_call_basic_block;
        delete extern_call_store_basic_block;
    }

    std::string get_function_name();

    MFunc *get_mfunction();

    void set_function(MFunc *mfunction);

//    virtual void codegen() = 0;
    virtual void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb, ExternCallBasicBlock *ecbb,
                                        ExternCallStoreBasicBlock *ecsbb) = 0;

    void base_codegen() {
        // initialize the function args
        extern_arg_prep_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_arg_prep_basic_block->set_extern_function(mfunction);
        extern_arg_prep_basic_block->codegen(jit);
        jit->get_builder().CreateBr(loop->get_loop_counter_basic_block()->get_basic_block());

        // loop components
        loop->set_max_loop_bound(llvm::cast<llvm::AllocaInst>(last(extern_arg_prep_basic_block->get_args())));
        loop->set_branch_to_after_counter(return_struct_basic_block->get_basic_block());
        loop->set_branch_to_true_condition(extern_init_basic_block->get_basic_block());
        loop->set_branch_to_false_condition(for_loop_end_basic_block->get_basic_block());
        loop->codegen();

        // allocate space for the return structure
        return_struct_basic_block->set_function(mfunction->get_extern_wrapper());
        return_struct_basic_block->set_extern_function(mfunction);
        if (mfunction->get_associated_block() == "ComparisonBlock") {
            // output size is N^2 where N is the loop bound
            llvm::Value *mult = jit->get_builder().CreateMul(loop->get_loop_counter_basic_block()->get_loop_bound(),
                                                             loop->get_loop_counter_basic_block()->get_loop_bound());
            llvm::AllocaInst *mult_alloc = jit->get_builder().CreateAlloca(mult->getType());
            jit->get_builder().CreateStore(mult, mult_alloc)->setAlignment(8);
            return_struct_basic_block->set_max_num_ret_elements(mult_alloc);
        } else {
            return_struct_basic_block->set_max_num_ret_elements(loop->get_loop_counter_basic_block()->get_loop_bound());
        }
        return_struct_basic_block->set_stage_return_type(mfunction->get_extern_wrapper_data_ret_type());
        return_struct_basic_block->codegen(jit);
        jit->get_builder().CreateBr(loop->get_for_loop_condition_basic_block()->get_basic_block());

        // codegen the body for the appropriate stage
        stage_specific_codegen(extern_arg_prep_basic_block->get_args(), extern_init_basic_block,
                               extern_call_basic_block, extern_call_store_basic_block);

        // store the result
        extern_call_store_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_call_store_basic_block->set_mtype(mfunction->get_extern_wrapper_data_ret_type());
        extern_call_store_basic_block->set_data_to_store(extern_call_basic_block->get_data_to_return());
        extern_call_store_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
        extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
        extern_call_store_basic_block->codegen(jit);
        jit->get_builder().CreateBr(loop->get_for_loop_increment_basic_block()->get_basic_block());

        // return the data
        for_loop_end_basic_block->set_function(mfunction->get_extern_wrapper());
        for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
        for_loop_end_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
        for_loop_end_basic_block->codegen(jit);

        mfunction->verify_wrapper();
    }

    mtype_code_t get_input_mtype_code();

    mtype_code_t get_output_mtype_code();

//    LoopCounterBasicBlock *get_loop_counter_basic_block();

    ReturnStructBasicBlock *get_return_struct_basic_block();

//    ForLoopConditionBasicBlock *get_for_loop_condition_basic_block();

//    ForLoopIncrementBasicBlock *get_for_loop_increment_basic_block();

    ForLoopEndBasicBlock *get_for_loop_end_basic_block();

    ExternInitBasicBlock *get_extern_init_basic_block();

    ExternArgPrepBasicBlock *get_extern_arg_prep_basic_block();

    ExternCallBasicBlock *get_extern_call_basic_block();

    ExternCallStoreBasicBlock *get_extern_call_store_basic_block();

//    virtual void postprocess(Stage *stage, llvm::BasicBlock *branch_from, llvm::BasicBlock *branch_into) = 0;

};

#endif //MATCHIT_STAGE_H
