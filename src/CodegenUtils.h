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

namespace CodegenUtils {

std::vector<llvm::AllocaInst *> load_wrapper_input_args(JIT *jit, llvm::Function *function);

std::vector<llvm::AllocaInst *> load_extern_input_arg(JIT *jit, std::vector<llvm::AllocaInst *> wrapper_input_arg_alloc,
                                                      llvm::AllocaInst *preallocated_output_space,
                                                      std::vector<llvm::AllocaInst *> loop_idx,
                                                      bool is_segmentation_stage, bool has_output_param,
                                                      llvm::AllocaInst *output_idx);

llvm::AllocaInst * create_extern_call(JIT *jit, llvm::Function *extern_function,
                                      std::vector<llvm::AllocaInst *> extern_arg_allocs);

llvm::AllocaInst *init_i64(JIT *jit, int start_val = 0, std::string name = "");

void increment_i64(JIT *jit, llvm::AllocaInst *i64_val, int step_size = 1);

llvm::Value * create_loop_condition_check(JIT *jit, llvm::AllocaInst *loop_idx_alloc, llvm::AllocaInst *max_loop_bound);

llvm::AllocaInst *init_wrapper_output_struct(JIT *jit, MFunc *mfunction, llvm::AllocaInst *max_loop_bound,
                                             llvm::AllocaInst *malloc_size);

void return_data(JIT *jit, llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *output_idx);

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src, llvm::Value *bytes_to_copy);

llvm::Value *codegen_c_malloc32(JIT *jit, size_t size);

llvm::Value *codegen_c_malloc32(JIT *jit, llvm::Value *size);

llvm::Value *codegen_c_malloc64(JIT *jit, size_t size);

llvm::Value *codegen_c_malloc64(JIT *jit, llvm::Value *size);

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_realloc(JIT *jit, llvm::Function *c_realloc, llvm::LoadInst *loaded_structure, llvm::Value *size);

llvm::Value *codegen_c_realloc32(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size);

llvm::Value *codegen_c_realloc64(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size);

llvm::Value *codegen_c_realloc32_and_cast(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_c_realloc64_and_cast(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size, llvm::Type *cast_to);

void codegen_fprintf_int(JIT *jit, llvm::Value *the_int);

void codegen_fprintf_int(JIT *jit, int the_int);

void codegen_fprintf_float(JIT *jit, llvm::Value *the_int);

void codegen_fprintf_float(JIT *jit, float the_int);

llvm::Value *codegen_llvm_ceil(JIT *jit, llvm::Value *ceil_me);

// some useful things
llvm::ConstantInt *get_i1(int zero_or_one);

llvm::ConstantInt *get_i32(int x);

llvm::ConstantInt *get_i64(long x);

llvm::Value *as_i32(JIT *jit, llvm::Value *i);

llvm::Value *gep(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx);

llvm::LoadInst *gep_and_load(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx);

}

#endif //MATCHIT_CODEGENUTILS_H
