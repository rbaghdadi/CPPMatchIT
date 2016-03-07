//
// Created by Jessica Ray on 1/28/16.
//

#include <stdio.h>
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "./JIT.h"
#include "./Pipeline.h"
#include "./CodegenUtils.h"

void Pipeline::register_stage(Stage *stage) {
    stages.push_back(stage);
}

void Pipeline::simple_execute(JIT *jit, const void **data) {
    jit->run("pipeline", data);
}

void Pipeline::codegen(JIT *jit, size_t num_prim_values, size_t num_structs) {

    assert(!stages.empty());

    /*
     * Create the pipeline wrapper
     */

    stages[0]->codegen();
    MFunc *mfunction = stages[0]->get_mfunction();
    llvm::Function *llvm_extern_wrapper = mfunction->get_extern_wrapper();

    // the argument types of the llvm function that we want to call
    std::vector<llvm::Type *> pipeline_arg_types;
    pipeline_arg_types.push_back(llvm::PointerType::get(mfunction->get_extern_param_types()[0]->codegen_type(), 0));

    // our pipeline wrapper function
    // add the return type
    llvm::FunctionType *llvm_func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                 pipeline_arg_types, false);
    // create the function
    llvm::Function *wrapper = llvm::Function::Create(llvm_func_type, llvm::Function::ExternalLinkage, "pipeline", jit->get_module().get());
    llvm::BasicBlock *wrapper_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry_block", wrapper);
    jit->get_builder().SetInsertPoint(wrapper_block);

    // give names to the inputs of our wrapper function
    int iter_ctr = 0;
    for (llvm::Function::arg_iterator iter = wrapper->arg_begin(); iter != wrapper->arg_end(); iter++) {
        llvm::Value *arg = iter;
        arg->setName("pipe_x" + std::to_string(iter_ctr++));
    }

    /*
     * Prep the input data that will be fed into the first block
     */

    std::vector<llvm::Value *> llvm_func_args;
    iter_ctr = 0;
    for (llvm::Function::arg_iterator iter = wrapper->arg_begin(); iter != wrapper->arg_end(); iter++) {
        // allocate space and load the arguments
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(mfunction->get_extern_wrapper_param_types()[iter_ctr]->codegen_type());
        alloc->setAlignment(8);
        jit->get_builder().CreateStore(iter, alloc)->setAlignment(8);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(alloc);
        load->setAlignment(8);
        llvm_func_args.push_back(load);
        iter_ctr++;
    }
    llvm_func_args.push_back(CodegenUtils::get_i64(num_prim_values));
    llvm_func_args.push_back(CodegenUtils::get_i64(num_structs));

    /*
     * Create the stage calls and chain them together
     */

    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *call = jit->get_builder().CreateCall(llvm_extern_wrapper, llvm_func_args);

    // print a separator to make the output easier to parse
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    Stage *prev_block = stages[0];
    for (std::vector<Stage *>::iterator iter = stages.begin() + 1; iter != stages.end(); iter++) {
        (*iter)->codegen();
        jit->get_builder().SetInsertPoint(wrapper_block);
        // split out the fields of the returned struct
        llvm::LoadInst *field_one = CodegenUtils::gep_and_load(jit, call, 0, 0);
        llvm::LoadInst *field_two = CodegenUtils::gep_and_load(jit, call, 0, 1);
        llvm::LoadInst *field_three = CodegenUtils::gep_and_load(jit, call, 0, 2);
        std::vector<llvm::Value *> args;
        args.push_back(field_one);
        args.push_back(field_two);
        args.push_back(field_three);
        call = jit->get_builder().CreateCall((*iter)->get_mfunction()->get_extern_wrapper(), args);
        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
        prev_block = (*iter);
    }

    jit->get_builder().CreateRet(nullptr);
    mfunction->verify_wrapper();
}