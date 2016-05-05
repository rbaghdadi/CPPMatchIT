//
// Created by Jessica Ray on 4/19/16.
//

#ifndef MATCHIT_INIT_H
#define MATCHIT_INIT_H

#include "./LLVM.h"
#include "./JIT.h"
#include "./Utils.h"
#include "./Field.h"

JIT *init() {
    JIT *jit = LLVM::init();
    register_utils(jit);
    init_element(jit);
    return jit;
}

void cleanup(JIT *jit) {
    delete jit;
}

#endif //MATCHIT_INIT_H
