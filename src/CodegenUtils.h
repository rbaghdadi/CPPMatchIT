//
// Created by Jessica Ray on 1/22/16.
//

// A collection of common codegen stuff for LLVM IR

#ifndef MATCHIT_CODEGENUTILS_H
#define MATCHIT_CODEGENUTILS_H

#include "./JIT.h"
#include "./MFunc.h"

class LLVMIR {

public:

    static llvm::Value *zero32;
    static llvm::Value *zero64;

    static llvm::Value *one32;
    static llvm::Value *one64;

    /*
     * Branches
     */

    /**
     * Create an unconditional branch from the current position to branch_to
     */
    static void branch(JIT *jit, llvm::BasicBlock *branch_to);

    /**
     * Create a conditional branch from the current position to true_branch or false_branch based on condition
     */
    static void branch_cond(JIT *jit, llvm::Value *condition, llvm::BasicBlock *true_branch,
                            llvm::BasicBlock *false_branch);

    /*
     * Generic for loop components
     */

    /**
     * Initialize the loop counter of a for loop
     */
    static llvm::AllocaInst *init_loop_idx(JIT *jit, int start_idx);

    /**
    * Increment the loop counter
    */
    static void build_loop_inc_block(JIT *jit, llvm::AllocaInst *alloc, int start_idx, int step_size);

    /**
     * Check the loop counter against the end index
     */
    static llvm::Value *build_loop_cond_block(JIT *jit, llvm::AllocaInst *alloc_cur_loop_idx,
                                              llvm::AllocaInst *max_bound);

    static void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src);

    static llvm::Value *codegen_c_malloc(JIT *jit, size_t size);

    static llvm::Value *codegen_c_malloc_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

    static void codegen_ret_idx_inc(JIT *jit, llvm::AllocaInst *alloc_ret_idx, int step_size = 1);

    static llvm::LoadInst *codegen_load_struct_field(JIT *jit, llvm::AllocaInst *struct_alloc, int field_idx);

    static void codegen_store_malloc_in_struct(JIT *jit, llvm::LoadInst *dest, llvm::Value *malloc_space, llvm::AllocaInst *index) {
        llvm::LoadInst *idx = jit->get_builder().CreateLoad(index);
        idx->setAlignment(8);
        std::vector<llvm::Value *> gep_element_idx;
        gep_element_idx.push_back(idx);
        llvm::Value *gep_element = jit->get_builder().CreateInBoundsGEP(dest, gep_element_idx);
        llvm::StoreInst *store_malloc = jit->get_builder().CreateStore(malloc_space, gep_element);
    }

};
#endif //MATCHIT_CODEGENUTILS_H
