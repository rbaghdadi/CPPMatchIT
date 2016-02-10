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

void Pipeline::codegen(JIT *jit, size_t size) {

    assert(!stages.empty());

    /*
     * Create the pipeline wrapper
     */

    // these both reference the same function, but give us two different sets of data
    stages[0]->codegen();
    MFunc *m_extern_wrapper = stages[0]->get_mfunction();
    llvm::Function *llvm_extern_wrapper = m_extern_wrapper->get_extern_wrapper();

    // the argument types of the llvm function that we want to call
    std::vector<llvm::Type *> llvm_func_arg_types;
    for (size_t i = 0; i < m_extern_wrapper->get_param_types().size(); i++) {
        llvm_func_arg_types.push_back(llvm::PointerType::get(llvm::PointerType::get(m_extern_wrapper->get_param_types()[i]->codegen(), 0), 0));
    }

    // our pipeline wrapper function
    // add the return type
    llvm::FunctionType *llvm_func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                 llvm_func_arg_types, false);
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
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(llvm::PointerType::get(llvm::PointerType::get(
                m_extern_wrapper->get_param_types()[iter_ctr]->codegen(), 0), 0));
        alloc->setAlignment(8);
        jit->get_builder().CreateStore(iter, alloc)->setAlignment(8);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(alloc);
        load->setAlignment(8);
        llvm_func_args.push_back(load);
        iter_ctr++;
    }
    llvm_func_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), size));


    /*
     * Create the block calls and chain them together
     */

    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *call = jit->get_builder().CreateCall(llvm_extern_wrapper, llvm_func_args);

    // print a separator to make the output easier to parse
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    Stage *prev_block = stages[0];
    for (std::vector<Stage *>::iterator iter = stages.begin() + 1; iter != stages.end(); iter++) {
        (*iter)->codegen();
        jit->get_builder().SetInsertPoint(wrapper_block);

        // alloc space for the result of the previous call
        llvm::AllocaInst *call_alloc = jit->get_builder().CreateAlloca(call->getType());

        // store the result of the previous call
        jit->get_builder().CreateStore(call, call_alloc);

        // hey, now load it again
        llvm::LoadInst *call_load = jit->get_builder().CreateLoad(call_alloc);



//        // allocate space for the MArray
//        std::vector<llvm::Value *> marray_gep_idxs;
//        marray_gep_idxs.push_back(CodegenUtils::get_i32(0));
//        llvm::Value *marray_gep_temp = jit->get_builder().CreateInBoundsGEP(call_alloc, marray_gep_idxs);
//        llvm::LoadInst *marray_gep_temp_load = jit->get_builder().CreateLoad(marray_gep_temp);
//        marray_gep_idxs.push_back(CodegenUtils::get_i32(0));
//        llvm::Value *marray_gep_ptr = jit->get_builder().CreateInBoundsGEP(marray_gep_temp_load, marray_gep_idxs);

        // get X**
        std::vector<llvm::Value *> x_gep_idxs;
        x_gep_idxs.push_back(CodegenUtils::get_i32(0));
        llvm::Value *x_gep_temp = jit->get_builder().CreateInBoundsGEP(call_alloc, x_gep_idxs);
        llvm::LoadInst *x_gep_temp_load = jit->get_builder().CreateLoad(x_gep_temp);
        x_gep_idxs.push_back(CodegenUtils::get_i32(0));
        llvm::Value *x_gep_ptr = jit->get_builder().CreateInBoundsGEP(x_gep_temp_load, x_gep_idxs);
        llvm::LoadInst *x_gep_ptr_load = jit->get_builder().CreateLoad(x_gep_ptr);
        // while we are here, get the value stored in the second marray field
        std::vector<llvm::Value *> field2_gep_idxs;
        field2_gep_idxs.push_back(CodegenUtils::get_i32(0));
        field2_gep_idxs.push_back(CodegenUtils::get_i32(1));
        llvm::Value *field2_gep = jit->get_builder().CreateInBoundsGEP(x_gep_ptr_load, field2_gep_idxs);
        llvm::LoadInst *num_inner_objs = jit->get_builder().CreateLoad(field2_gep);
        // now back to getting our X**
        llvm::Value *x_gep = jit->get_builder().CreateInBoundsGEP(x_gep_ptr_load, x_gep_idxs);
        llvm::LoadInst *x_gep_load = jit->get_builder().CreateLoad(x_gep);

        // load the returned fields
//        std::vector<llvm::Value *> ret_val_idx; // user type
//        ret_val_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        ret_val_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        llvm::Value *gep_ret_val = jit->get_builder().CreateInBoundsGEP(call_load, ret_val_idx);
//        llvm::LoadInst *loaded_val = jit->get_builder().CreateLoad(gep_ret_val);
//
//        std::vector<llvm::Value *> ret_idx_idx; // counter
//        ret_idx_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        ret_idx_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//        llvm::Value *gep_ret_idx = jit->get_builder().CreateInBoundsGEP(call_load, ret_idx_idx);
//        llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(gep_ret_idx);

        // now call the next function
        std::vector<llvm::Value *> args;
        args.push_back(x_gep_load);
        if ((*iter)->get_mfunction()->get_associated_block().compare("ComparisonStage") == 0) {
            args.push_back(x_gep_load);
        }
        args.push_back(jit->get_builder().CreateSExtOrBitCast(num_inner_objs, llvm::Type::getInt64Ty(llvm::getGlobalContext())));

        call = jit->get_builder().CreateCall((*iter)->get_mfunction()->get_extern_wrapper(), args);
//        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

        prev_block = (*iter);
    }

    jit->get_builder().CreateRet(nullptr);

    m_extern_wrapper->verify_wrapper();

}