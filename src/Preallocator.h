//
// Created by Jessica Ray on 3/21/16.
//

#ifndef MATCHIT_PREALLOCATOR_H
#define MATCHIT_PREALLOCATOR_H

#include "./JIT.h"
#include "./ForLoop.h"
#include "./Field.h"

llvm::AllocaInst *preallocate_field(JIT *jit, BaseField *base_field, llvm::Value *total_space_to_allocate);
//llvm::AllocaInst *do_malloc(JIT *jit, llvm::Type *alloca_type, llvm::Value *size_to_malloc, std::string name = "");
//
//void preallocate_loop(JIT *jit, ForLoop *loop, llvm::Value *num_structs, llvm::Function *stage_function,
//                      llvm::BasicBlock *loop_body, llvm::BasicBlock *dummy);
//
//void divide_preallocated_struct_space(JIT *jit, llvm::AllocaInst *preallocated_struct_ptr_ptr_space,
//                                      llvm::AllocaInst *preallocated_struct_ptr_space,
//                                      llvm::AllocaInst *preallocated_T_ptr_space, llvm::AllocaInst *loop_idx,
//                                      llvm::Value *T_ptr_idx, MType *data_mtype);
//
//llvm::AllocaInst *preallocate(BaseField *base, JIT *jit, llvm::Value *num_set_elements, llvm::Function *function);

#endif //MATCHIT_PREALLOCATOR_H
