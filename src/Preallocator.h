//
// Created by Jessica Ray on 3/21/16.
//

#ifndef MATCHIT_PREALLOCATOR_H
#define MATCHIT_PREALLOCATOR_H

#include "./JIT.h"
#include "./Field.h"

llvm::AllocaInst *preallocate_field(JIT *jit, BaseField *base_field, llvm::Value *total_space_to_allocate);

#endif //MATCHIT_PREALLOCATOR_H
