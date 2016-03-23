//
// Created by Jessica Ray on 1/22/16.
//

// A collection of common codegen_old stuff for LLVM IR

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

std::vector<llvm::AllocaInst *> load_user_function_input_arg(JIT *jit,
                                                             std::vector<llvm::AllocaInst *> stage_input_arg_alloc,
                                                             llvm::AllocaInst *preallocated_output_space,
                                                             std::vector<llvm::AllocaInst *> loop_idx,
                                                             bool is_segmentation_stage, bool has_output_param,
                                                             llvm::AllocaInst *output_idx);

llvm::AllocaInst * create_extern_call(JIT *jit, llvm::Function *extern_function,
                                      std::vector<llvm::AllocaInst *> extern_arg_allocs);

llvm::AllocaInst *init_i64(JIT *jit, int start_val = 0, std::string name = "");

llvm::AllocaInst *init_i32(JIT *jit, int start_val = 0, std::string name = "");

void increment_i64(JIT *jit, llvm::AllocaInst *i64_val, int step_size = 1);

void increment_i32(JIT *jit, llvm::AllocaInst *i32_val, int step_size = 1);

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

llvm::Value *gep_i64_i32(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx);

//llvm::LoadInst *gep_i64_i32_and_load(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx);

llvm::Value *codegen_llvm_gep(JIT *jit, llvm::Value *gep_this, std::vector<llvm::Value *> gep_idxs);

llvm::AllocaInst *codegen_llvm_alloca(JIT *jit, llvm::Type *type, unsigned int alignment, std::string name = "");

llvm::StoreInst *codegen_llvm_store(JIT *jit, llvm::Value *src, llvm::Value *dest, unsigned int alignment);

llvm::LoadInst *codegen_llvm_load(JIT *jit, llvm::Value *src, unsigned int alignment);

llvm::Type *codegen_llvm_ptr(JIT *jit, llvm::Type *element_type);

llvm::Value *codegen_llvm_mul(JIT *jit, llvm::Value *left, llvm::Value *right);

static llvm::Type *llvm_void = llvm::Type::getVoidTy(llvm::getGlobalContext());

static llvm::Type *llvm_float = llvm::Type::getFloatTy(llvm::getGlobalContext());

static llvm::Type *llvm_int1 = llvm::Type::getInt1Ty(llvm::getGlobalContext());

static llvm::Type *llvm_int8ptr = llvm::Type::getInt8PtrTy(llvm::getGlobalContext());

static llvm::Type *llvm_int32 = llvm::Type::getInt32Ty(llvm::getGlobalContext());

static llvm::Type *llvm_int64 = llvm::Type::getInt64Ty(llvm::getGlobalContext());

llvm::Value *codegen_llvm_add(JIT *jit, llvm::Value *left, llvm::Value *right);

}

#endif //MATCHIT_CODEGENUTILS_H
