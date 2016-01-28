//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "./LLVM.h"

bool LLVM::initialized = false;

void LLVM::init() {
    if (!initialized) {
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();
        LLVMInitializeNativeAsmParser();
        initialized = true;
    }
}
