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
    bool codegen_done = false;

    WrapperOutputStructIB *return_struct;
    ForLoopEndIB *for_loop_end;
    ExternArgLoaderIB *extern_init;
    WrapperArgLoaderIB *load_input_args;
    ExternCallIB *extern_call;
    ExternCallStoreIB *extern_call_store;

public:

    Stage(JIT *jit, mtype_code_t input_type_code, mtype_code_t output_type_code, std::string function_name) :
            jit(jit), function_name(function_name), input_mtype_code(input_type_code),
            output_mtype_code(output_type_code) {
        return_struct = new WrapperOutputStructIB();
        for_loop_end = new ForLoopEndIB();
        extern_init = new ExternArgLoaderIB();
        load_input_args = new WrapperArgLoaderIB();
        extern_call = new ExternCallIB();
        extern_call_store = new ExternCallStoreIB();
    }

    virtual ~Stage() { }

    std::string get_function_name();

    MFunc *get_mfunction();

    void set_function(MFunc *mfunction);

    virtual void init_codegen() = 0;

    virtual void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternArgLoaderIB *eibb,
                                        ExternCallIB *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) = 0;

    virtual llvm::BasicBlock *branch_to_after_store();

    virtual void codegen();

    void set_for_loop(ForLoop *loop);

    mtype_code_t get_input_mtype_code();

    mtype_code_t get_output_mtype_code();

    JIT *get_jit();

    WrapperOutputStructIB *get_return_struct();

    ForLoopEndIB *get_for_loop_end();

    ExternArgLoaderIB *get_extern_init();

    WrapperArgLoaderIB *get_load_input_args();

    ExternCallIB *get_extern_call();

    ExternCallStoreIB *get_extern_call_store();

};

#endif //MATCHIT_STAGE_H
