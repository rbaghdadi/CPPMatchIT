//
// Created by Jessica Ray on 3/21/16.
//

#include "./Preallocator.h"
#include "./CodegenUtils.h"

using namespace Codegen;

llvm::AllocaInst *preallocate_field(JIT *jit, BaseField *base_field, llvm::Value *total_space_to_allocate) {
    llvm::AllocaInst *allocated_space = codegen_llvm_alloca(jit, llvm_int8Ptr, 8);
    llvm::Value *space = codegen_mmalloc(jit, total_space_to_allocate);
    codegen_llvm_store(jit, space, allocated_space, 8);
    return allocated_space;
}
