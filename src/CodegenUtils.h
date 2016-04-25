//
// Created by Jessica Ray on 1/22/16.
//

// A collection of common codegen_old stuff for LLVM IR

#ifndef MATCHIT_CODEGENUTILS_H
#define MATCHIT_CODEGENUTILS_H

#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"
#include "./InstructionBlock.h"
#include "./JIT.h"
#include "./MFunc.h"
#include "./MType.h"

namespace Codegen {

/*
 * C memory wrappers
 */

llvm::Value *codegen_mmalloc(JIT *jit, size_t size);

llvm::Value *codegen_mmalloc(JIT *jit, llvm::Value *size);

llvm::Value *codegen_mmalloc_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_mmalloc_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_mrealloc(JIT *jit, llvm::Function *mrealloc, llvm::LoadInst *structure, llvm::Value *size);

llvm::Value *codegen_mrealloc(JIT *jit, llvm::LoadInst *structure, llvm::Value *size);

llvm::Value *codegen_mrealloc_and_cast(JIT *jit, llvm::LoadInst *structure, llvm::Value *size, llvm::Type *cast_to);

void codegen_mfree(JIT *jit, llvm::LoadInst *structure);

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src, llvm::Value *bytes_to_copy);

/*
 * C fprintf wrappers
 */

void codegen_fprintf_int(JIT *jit, llvm::Value *the_int);

void codegen_fprintf_int(JIT *jit, int the_int);

void codegen_fprintf_float(JIT *jit, llvm::Value *the_int);

void codegen_fprintf_float(JIT *jit, float the_int);

/*
 * LLVM primitive types
 */

static llvm::Type *llvm_void = llvm::Type::getVoidTy(llvm::getGlobalContext());

static llvm::Type *llvm_float = llvm::Type::getFloatTy(llvm::getGlobalContext());

static llvm::PointerType *llvm_floatPtr = llvm::Type::getFloatPtrTy(llvm::getGlobalContext());

static llvm::IntegerType *llvm_int1 = llvm::Type::getInt1Ty(llvm::getGlobalContext());

static llvm::PointerType *llvm_int1Ptr = llvm::Type::getInt1PtrTy(llvm::getGlobalContext());

static llvm::IntegerType *llvm_int8 = llvm::Type::getInt8Ty(llvm::getGlobalContext());

static llvm::PointerType *llvm_int8Ptr = llvm::Type::getInt8PtrTy(llvm::getGlobalContext());

static llvm::IntegerType *llvm_int16 = llvm::Type::getInt16Ty(llvm::getGlobalContext());

static llvm::PointerType *llvm_int16Ptr = llvm::Type::getInt16PtrTy(llvm::getGlobalContext());

static llvm::IntegerType *llvm_int32 = llvm::Type::getInt32Ty(llvm::getGlobalContext());

static llvm::PointerType *llvm_int32Ptr = llvm::Type::getInt32PtrTy(llvm::getGlobalContext());

static llvm::IntegerType *llvm_int64 = llvm::Type::getInt64Ty(llvm::getGlobalContext());

static llvm::PointerType *llvm_int64Ptr = llvm::Type::getInt64PtrTy(llvm::getGlobalContext());

/*
 * C to LLVM conversions
 */

llvm::ConstantFP * as_float(float x);

llvm::ConstantInt *as_i1(bool x);

llvm::ConstantInt *as_i8(char x);

llvm::ConstantInt *as_i16(short x);

llvm::ConstantInt *as_i32(int x);

llvm::ConstantInt *as_i64(long x);

/*
 * LLVM conversions
 */

llvm::Type *codegen_llvm_ptr(JIT *jit, llvm::Type *element_type);

/*
 * Wrappers for LLVM functions
 */

llvm::AllocaInst *codegen_llvm_alloca(JIT *jit, llvm::Type *type, unsigned int alignment, std::string name = "");

llvm::StoreInst *codegen_llvm_store(JIT *jit, llvm::Value *src, llvm::Value *dest, unsigned int alignment);

llvm::LoadInst *codegen_llvm_load(JIT *jit, llvm::Value *src, unsigned int alignment);

llvm::Value *codegen_llvm_ceil(JIT *jit, llvm::Value *ceil_me);

llvm::Value *codegen_llvm_add(JIT *jit, llvm::Value *left, llvm::Value *right);

llvm::Value *codegen_llvm_mul(JIT *jit, llvm::Value *left, llvm::Value *right);

llvm::Value *gep_i64_i32(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx);

llvm::Value *codegen_llvm_gep(JIT *jit, llvm::Value *gep_this, std::vector<llvm::Value *> gep_idxs);

/*
 * Stage codegen stuff
 */

std::vector<llvm::AllocaInst *> load_stage_input_params(JIT *jit, llvm::Function *stage);

std::vector<llvm::AllocaInst *> load_user_function_input_param(JIT *jit,
                                                               std::vector<llvm::AllocaInst *> stage_input_param_alloc,
                                                               llvm::AllocaInst *preallocated_output_space,
                                                               std::vector<llvm::AllocaInst *> loop_idx,
                                                               bool is_segmentation_stage, bool is_filter_stage,
                                                               bool has_output_param, llvm::AllocaInst *output_idx);

llvm::AllocaInst *create_user_function_call(JIT *jit, llvm::Function *user_function,
                                            std::vector<llvm::AllocaInst *> user_function_param_allocs);

llvm::Value *create_loop_condition_check(JIT *jit, llvm::AllocaInst *loop_idx_alloc, llvm::AllocaInst *max_loop_bound);

}

#endif //MATCHIT_CODEGENUTILS_H
