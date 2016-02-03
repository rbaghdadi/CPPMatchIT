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

void Pipeline::simple_execute(JIT *jit, std::vector<File *> data) {
//    std::cerr << data.size() << std::endl;
//    char **void_data = (char**)malloc(sizeof(char*) * data.size());

    std::vector<const void *> vecdata;

    int ctr = 0;
    for (std::vector<File *>::iterator iter = data.begin(); iter != data.end(); iter++) {
//        void_data[ctr] = (char*)malloc(sizeof(char) * (*iter)->get_size());
//        strcpy(void_data[ctr++], (*iter)->get_underlying_array());
//        void_data[ctr++] = (*iter)->get_underlying_array();
//        std::cerr << void_data[ctr - 1] << std::endl;

        vecdata.push_back((*iter)->get_underlying_array());
        std::cerr << (char*)vecdata[vecdata.size() - 1] << std::endl;
    }
    jit->run("pipeline", &(vecdata[0]));//(const void **)void_data);
}

void Pipeline::codegen(JIT *jit, size_t size) {

    assert(!stages.empty());

    /*
     * Create the pipeline wrapper
     */

    // these both reference the same function, but give us two different sets of data
    stages[0]->base_codegen();
    MFunc *m_extern_wrapper = stages[0]->get_mfunction();
    llvm::Function *llvm_extern_wrapper = m_extern_wrapper->get_extern_wrapper();

    // the argument types of the llvm function that we want to call
    std::vector<llvm::Type *> llvm_func_arg_types;
    for (size_t i = 0; i < m_extern_wrapper->get_arg_types().size(); i++) {
        llvm_func_arg_types.push_back(llvm::PointerType::get(m_extern_wrapper->get_arg_types()[i]->codegen(), 0));
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
        unsigned int alignment = m_extern_wrapper->get_arg_types()[iter_ctr]->get_alignment();
        // allocate space and load the arguments
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(llvm::PointerType::get(m_extern_wrapper->get_arg_types()[iter_ctr]->codegen(), 0));
        alloc->setAlignment(alignment);
//
//        llvm::LoadInst *load_alloc = jit->get_builder().CreateLoad(alloc);
//        llvm::Value *malloc_space = CodegenUtils::codegen_c_malloc_and_cast(jit, 16, alloc->getType());
//        jit->get_builder().CreateStore(malloc_space, alloc);
//        llvm::LoadInst *loaded_element = jit->get_builder().CreateLoad(iter);
//        llvm::LoadInst *load_alloc = jit->get_builder().CreateLoad(alloc);
//        CodegenUtils::codegen_llvm_memcpy(jit, loaded_element, load_alloc);


        llvm::StoreInst *store = jit->get_builder().CreateStore(iter, alloc);
        store->setAlignment(alignment);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(alloc);
        load->setAlignment(alignment);
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
        (*iter)->base_codegen();
        jit->get_builder().SetInsertPoint(wrapper_block);

        // alloc space for the result of the previous call
        llvm::AllocaInst *call_alloc = jit->get_builder().CreateAlloca(call->getType());

        // store the result of the previous call
        jit->get_builder().CreateStore(call, call_alloc);

        // hey, now load it again
        llvm::LoadInst *call_load = jit->get_builder().CreateLoad(call_alloc);

        // load the returned fields
        std::vector<llvm::Value *> ret_val_idx; // user type
        ret_val_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
        ret_val_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
        llvm::Value *gep_ret_val = jit->get_builder().CreateInBoundsGEP(call_load, ret_val_idx);
        llvm::LoadInst *loaded_val = jit->get_builder().CreateLoad(gep_ret_val);

        std::vector<llvm::Value *> ret_idx_idx; // counter
        ret_idx_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
        ret_idx_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
        llvm::Value *gep_ret_idx = jit->get_builder().CreateInBoundsGEP(call_load, ret_idx_idx);
        llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(gep_ret_idx);

        // now call the next function
        std::vector<llvm::Value *> args;
        args.push_back(loaded_val);
        if ((*iter)->get_mfunction()->get_associated_block().compare("ComparisonStage") == 0) {
            args.push_back(loaded_val);
        }
        args.push_back(loaded_idx);

        call = jit->get_builder().CreateCall((*iter)->get_mfunction()->get_extern_wrapper(), args);
        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

        prev_block = (*iter);
    }

    jit->get_builder().CreateRet(nullptr);

    m_extern_wrapper->verify_wrapper();

}