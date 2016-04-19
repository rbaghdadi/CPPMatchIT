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

extern "C" void c_fprintf_float(float f) {
    fprintf(stderr, "%f\n", f);
}

extern "C" void *malloc_32(size_t size) {
    std::cerr << "Mallocing " << size << " bytes" << std::endl;
    return malloc(size);
}

extern "C" void *malloc_64(size_t size) {
    return malloc(size);
}

extern "C" void *realloc_32(void *structure, size_t size) {
    std::cerr << "Reallocing " << size << " bytes" << std::endl;
    return realloc(structure, size);
}

extern "C" void *realloc_64(void *structure, size_t size) {
    return realloc(structure, size);
}

void register_utils(JIT *jit) {
    /*
     * memcpy
     */
    std::vector<llvm::Type *> llvm_intr_memcpy_args;
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm_intr_memcpy_args.push_back(llvm::Type::getInt1Ty(llvm::getGlobalContext()));
    llvm::FunctionType *llvm_intr_memcpy_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                      llvm_intr_memcpy_args, false);
    jit->get_module()->getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i32", llvm_intr_memcpy_ft);

    /*
     * ceil
     */
    std::vector<llvm::Type *> llvm_intr_fceil_args;
    llvm_intr_fceil_args.push_back(llvm::Type::getFloatTy(llvm::getGlobalContext()));
    llvm::FunctionType *llvm_intr_fceil_ft = llvm::FunctionType::get(llvm::Type::getFloatTy(llvm::getGlobalContext()),
                                                                     llvm_intr_fceil_args, false);
    jit->get_module()->getOrInsertFunction("llvm.ceil.f32", llvm_intr_fceil_ft);

    /*
     * malloc32
     */
    // llvm is very strict, so we can't use the same malloc call for both 32 and 64 bit data
    std::vector<llvm::Type *> c_malloc32_args;
    c_malloc32_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_malloc32_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                c_malloc32_args, false);
    jit->get_module()->getOrInsertFunction("malloc_32", c_malloc32_ft);

    /*
     * malloc64
     */
    std::vector<llvm::Type *> c_malloc64_args;
    c_malloc64_args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_malloc64_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                c_malloc64_args, false);
    jit->get_module()->getOrInsertFunction("malloc_64", c_malloc64_ft);

    /*
     * realloc32
     */
    // llvm is very strict, so we can't use the same realloc call for both 32 and 64 bit data
    std::vector<llvm::Type *> c_realloc32_args;
    c_realloc32_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    c_realloc32_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_realloc32_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                 c_realloc32_args, false);
    jit->get_module()->getOrInsertFunction("realloc_32", c_realloc32_ft);

    /*
     * realloc64
     */
    std::vector<llvm::Type *> c_realloc64_args;
    c_realloc64_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    c_realloc64_args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_realloc64_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                 c_realloc64_args, false);
    jit->get_module()->getOrInsertFunction("realloc_64", c_realloc64_ft);

    /*
     * printf int
     */
    std::vector<llvm::Type *> c_print_int_args;
    c_print_int_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *c_print_int_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                 c_print_int_args, true);
    jit->get_module()->getOrInsertFunction("c_fprintf", c_print_int_ft);

    /*
     * printf float
     */
    std::vector<llvm::Type *> c_print_float_args;
    c_print_float_args.push_back(llvm::Type::getFloatTy(llvm::getGlobalContext()));
    llvm::FunctionType *c_print_float_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                   c_print_float_args, true);
    jit->get_module()->getOrInsertFunction("c_fprintf_float", c_print_float_ft);

    /*
     * print separator
     */
    llvm::FunctionType *print_sep_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                               std::vector<llvm::Type *>(), false);
    jit->get_module()->getOrInsertFunction("print_sep", print_sep_ft);

}