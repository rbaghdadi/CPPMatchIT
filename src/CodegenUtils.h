//
// Created by Jessica Ray on 1/22/16.
//

// A collection of common codegen stuff for LLVM IR

#ifndef MATCHIT_CODEGENUTILS_H
#define MATCHIT_CODEGENUTILS_H

#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"
#include "./MType.h"
#include "./JIT.h"
#include "./MFunc.h"
#include "./InstructionBlock.h"

llvm::Value *codegen_element_size(MType *mtype, llvm::AllocaInst *alloc_ret_data, JIT *jit);
llvm::Value *codegen_file_size(llvm::AllocaInst *alloc_ret_data, JIT *jit);
llvm::Value *codegen_comparison_element_size(MType *mtype, llvm::AllocaInst *alloc_ret_data, JIT *jit);
void codegen_file_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
                        llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
                        JIT *jit, llvm::Function *insert_into);

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

//    std::vector<llvm::Value *> slice(int start, int end) {
//        std::vector<llvm::Value *> sliced;
//        int ctr = 0;
//        for (int i = start; i < end; i++) {
//            sliced[ctr++] = results[i];
//        }
//        return sliced;
//    }

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

FuncComp create_extern_call(JIT *jit, MFunc extern_func, std::vector<llvm::Value *> extern_function_arg_allocs);

FuncComp init_idx(JIT *jit, int start_idx = 0, std::string name = "");

void increment_idx(JIT *jit, llvm::AllocaInst *loop_idx, int step_size = 1);

FuncComp create_loop_idx_condition(JIT *jit, llvm::AllocaInst *loop_idx, llvm::AllocaInst *loop_bound);

FuncComp init_return_data_structure(JIT *jit, MType *data_ret_type, MFunc extern_mfunc,
                                    llvm::Value *max_num_ret_elements, llvm::AllocaInst *malloc_size);

void store_extern_result(JIT *jit, MType *ret_type, llvm::Value *ret_struct, llvm::Value *ret_idx,
                         llvm::AllocaInst *extern_call_res, llvm::Function *insert_into, llvm::AllocaInst *malloc_size,
                         llvm::BasicBlock *extern_call_store_basic_block);

std::vector<llvm::AllocaInst *> init_function_args(JIT *jit, MFunc extern_mfunc);

void return_data(JIT *jit, llvm::Value *ret, llvm::Value *ret_idx);

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src, llvm::Value *bytes_to_copy);

llvm::Value *codegen_c_malloc32(JIT *jit, size_t size);

llvm::Value *codegen_c_malloc32(JIT *jit, llvm::Value *size);

llvm::Value *codegen_c_malloc64(JIT *jit, size_t size);

llvm::Value *codegen_c_malloc64(JIT *jit, llvm::Value *size);

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

void codegen_fprintf_int(JIT *jit, llvm::Value *the_int);

void codegen_fprintf_int(JIT *jit, int the_int);

}

#endif //MATCHIT_CODEGENUTILS_H
