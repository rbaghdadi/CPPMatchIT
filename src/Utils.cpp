//
// Created by Jessica Ray on 2/1/16.
//

#include "./Utils.h"

extern "C" void print_sep() {
    fprintf(stderr, "===========================================\n");
}

extern "C" void c_fprintf(int l) {
    fprintf(stderr, "%d\n", l);
}

extern "C" void print_float(float f) {
    fprintf(stderr, "%f\n", f);
}

extern "C" void *malloc_32(size_t size) {
    return malloc(size);
}

extern "C" void *malloc_64(size_t size) {
    return malloc(size);
}

extern "C" void *realloc_32(void *structure, size_t size) {
    return realloc(structure, size);
}

extern "C" void *realloc_64(void *structure, size_t size) {
    return realloc(structure, size);
}

void register_utils(JIT *jit) {
    std::vector<llvm::Type *> llvm_intr_memcpy_args;
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt1Ty(llvm::getGlobalContext()));
    llvm::FunctionType *llvm_intr_memcpy_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                      llvm_intr_memcpy_args, false);
    jit->get_module()->getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i32", llvm_intr_memcpy_ft);

    // llvm is very strict, so we can't use the same malloc call for both 32 and 64 bit data
    std::vector<llvm::Type *> c_malloc32_args;
    c_malloc32_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_malloc32_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                c_malloc32_args, false);
    jit->get_module()->getOrInsertFunction("malloc_32", c_malloc32_ft);

    std::vector<llvm::Type *> c_malloc64_args;
    c_malloc64_args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_malloc64_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                c_malloc64_args, false);
    jit->get_module()->getOrInsertFunction("malloc_64", c_malloc64_ft);

    // llvm is very strict, so we can't use the same realloc call for both 32 and 64 bit data
    std::vector<llvm::Type *> c_realloc32_args;
    c_realloc32_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    c_realloc32_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_realloc32_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                 c_realloc32_args, false);
    jit->get_module()->getOrInsertFunction("realloc_32", c_realloc32_ft);

    std::vector<llvm::Type *> c_realloc64_args;
    c_realloc64_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    c_realloc64_args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_realloc64_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                 c_realloc64_args, false);
    jit->get_module()->getOrInsertFunction("realloc_64", c_realloc64_ft);

    std::vector<llvm::Type *> c_print_int_args;
    c_print_int_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_print_int_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                 c_print_int_args, true);
    jit->get_module()->getOrInsertFunction("c_fprintf", c_print_int_ft);

    llvm::FunctionType *print_sep_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                               std::vector<llvm::Type *>(), false);
    jit->get_module()->getOrInsertFunction("print_sep", print_sep_ft);
}