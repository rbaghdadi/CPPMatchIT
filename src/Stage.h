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

class Stage {

protected:

    // these reference the wrapper functions, not the extern function that is what actually does work
    JIT *jit;
    MFunc *mfunction;
    std::string function_name;
    mtype_code_t input_mtype_code;
    mtype_code_t output_mtype_code;
    ForLoop *loop;

    ReturnStructBasicBlock *return_struct_basic_block;
    ForLoopEndBasicBlock *for_loop_end_basic_block;
    ExternInitBasicBlock *extern_init_basic_block;
    ExternArgPrepBasicBlock *extern_arg_prep_basic_block;
    ExternCallBasicBlock *extern_call_basic_block;
    ExternCallStoreBasicBlock *extern_call_store_basic_block;

public:

    Stage(JIT *jit, mtype_code_t input_type_code, mtype_code_t output_type_code, std::string function_name) :
            jit(jit), function_name(function_name), input_mtype_code(input_type_code), output_mtype_code(output_type_code) {
        return_struct_basic_block = new ReturnStructBasicBlock();
        for_loop_end_basic_block = new ForLoopEndBasicBlock();
        extern_init_basic_block = new ExternInitBasicBlock();
        extern_arg_prep_basic_block = new ExternArgPrepBasicBlock();
        extern_call_basic_block = new ExternCallBasicBlock();
        extern_call_store_basic_block = new ExternCallStoreBasicBlock();
    }

    virtual ~Stage() {
//        delete return_struct_basic_block;
//        delete for_loop_end_basic_block;
//        delete extern_init_basic_block;
//        delete extern_arg_prep_basic_block;
//        delete extern_call_basic_block;
//        delete extern_call_store_basic_block;
////        if (loop) {
////            delete loop;
////        }
//        if (mfunction) {
//            delete mfunction;
//        }
    }

    std::string get_function_name();

    MFunc *get_mfunction();

    void set_function(MFunc *mfunction);

    virtual void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb,
                                        ExternCallBasicBlock *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) = 0;

    virtual llvm::BasicBlock *branch_to_after_store();

    virtual void base_codegen();

//    virtual void get_input_element_size_in_bytes() = 0;
//
//    virtual void get_output_element_size_in_bytes() = 0;

    void set_for_loop(ForLoop *loop);

    mtype_code_t get_input_mtype_code();

    mtype_code_t get_output_mtype_code();

    JIT *get_jit();

    ReturnStructBasicBlock *get_return_struct_basic_block();

    ForLoopEndBasicBlock *get_for_loop_end_basic_block();

    ExternInitBasicBlock *get_extern_init_basic_block();

    ExternArgPrepBasicBlock *get_extern_arg_prep_basic_block();

    ExternCallBasicBlock *get_extern_call_basic_block();

    ExternCallStoreBasicBlock *get_extern_call_store_basic_block();

};

#endif //MATCHIT_STAGE_H
