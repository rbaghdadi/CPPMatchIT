//
// Created by Jessica Ray on 1/22/16.
//

// A collection of common codegen stuff for LLVM IR

#ifndef MATCHIT_CODEGENUTILS_H
#define MATCHIT_CODEGENUTILS_H

#include "./JIT.h"
#include "./MFunc.h"

namespace CodegenUtils {
// A component of a function. Can be anything really
// wraps LLVM IR
class FuncComp {
private:
    // where the result of this computation is stored
    std::vector<llvm::Value *> results;
public:
    FuncComp(llvm::Value *result) : results(std::vector<llvm::Value *>()) {
        this->results.push_back(result);
    }

    FuncComp(std::vector<llvm::Value *> results) : results(results) { }

    llvm::Value *get_result() {
        return results[0];
    }

    std::vector<llvm::Value *> get_results() {
        return results;
    }

    llvm::Value *get_result(unsigned int i) {
        return results[i];
    }

    size_t get_size() {
        return results.size();
    }

    llvm::Value *get_last() {
        return results[results.size() - 1];
    }
};

FuncComp create_extern_call(JIT *jit, MFunc extern_func, std::vector<llvm::Value *> extern_function_arg_addrs);

FuncComp init_idx(JIT *jit, int start_idx = 0, std::string name = "");

void increment_idx(JIT *jit, llvm::AllocaInst *loop_idx, int step_size = 1);

FuncComp check_loop_idx_condition(JIT *jit, llvm::AllocaInst *loop_idx, llvm::AllocaInst *loop_bound);

FuncComp init_return_data_structure(JIT *jit, MType *data_ret_type, MFunc extern_mfunc, llvm::Value *loop_bound);

void store_extern_result(JIT *jit, MType *ret_type, llvm::Value *ret, llvm::Value *ret_idx,
                         llvm::Value *extern_call_res);

FuncComp init_function_args(JIT *jit, MFunc extern_mfunc);

void return_data(JIT *jit, llvm::Value *ret, llvm::Value *ret_idx);

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src);

llvm::Value *codegen_c_malloc(JIT *jit, size_t size);

llvm::Value *codegen_c_malloc(JIT *jit, llvm::Value *size);

llvm::Value *codegen_c_malloc_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

}


//class LLVMIR {
//
//public:
//
//    static llvm::Value *zero32;
//    static llvm::Value *zero64;
//
//    static llvm::Value *one32;
//    static llvm::Value *one64;
//
//    /*
//     * Branches
//     */
//
//    /**
//     * Create an unconditional branch from the current position to branch_to
//     */
//    static void branch(JIT *jit, llvm::BasicBlock *branch_to);
//
//    /**
//     * Create a conditional branch from the current position to true_branch or false_branch based on condition
//     */
//    static void branch_cond(JIT *jit, llvm::Value *condition, llvm::BasicBlock *true_branch,
//                            llvm::BasicBlock *false_branch);
//
//    /*
//     * Generic for loop components
//     */
//
//    /**
//     * Initialize the loop counter of a for loop
//     */
//    static llvm::AllocaInst *init_loop_idx(JIT *jit, int start_idx);
//
//    /**
//    * Increment the loop counter
//    */
//    static void build_loop_inc_block(JIT *jit, llvm::AllocaInst *alloc, int start_idx, int step_size);
//
//    /**
//     * Check the loop counter against the end index
//     */
//    static llvm::Value *build_loop_cond_block(JIT *jit, llvm::AllocaInst *alloc_cur_loop_idx,
//                                              llvm::AllocaInst *max_bound);
//
//    static void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src);
//
//    static llvm::Value *codegen_c_malloc(JIT *jit, size_t size);
//
//    static llvm::Value *codegen_c_malloc(JIT *jit, llvm::Value *size);
//
//    static llvm::Value *codegen_c_malloc_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);
//
//    static llvm::Value *codegen_c_malloc_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);
//
//    static void codegen_ret_idx_inc(JIT *jit, llvm::AllocaInst *alloc_ret_idx, int step_size = 1);
//
//    static llvm::LoadInst *codegen_load_struct_field(JIT *jit, llvm::AllocaInst *struct_alloc, int field_idx);
//
//    static void codegen_store_malloc_in_struct(JIT *jit, llvm::LoadInst *dest, llvm::Value *malloc_space, llvm::AllocaInst *index) {
//        llvm::LoadInst *idx = jit->get_builder().CreateLoad(index);
//        idx->setAlignment(8);
//        std::vector<llvm::Value *> gep_element_idx;
//        gep_element_idx.push_back(idx);
//        llvm::Value *gep_element = jit->get_builder().CreateInBoundsGEP(dest, gep_element_idx);
//        llvm::StoreInst *store_malloc = jit->get_builder().CreateStore(malloc_space, gep_element);
//    }
//
//};
#endif //MATCHIT_CODEGENUTILS_H
