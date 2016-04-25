//
// Created by Jessica Ray on 2/1/16.
//

#include <sstream>
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

extern "C" void *mmalloc(size_t size) {
#ifdef PRINT_MALLOC
    std::cerr << "Mallocing " << size << " bytes" << std::endl;
#endif
    return malloc(size);
}

extern "C" void *mrealloc(void *structure, size_t size) {
#ifdef PRINT_MALLOC
    std::cerr << "Reallocing " << size << " bytes" << std::endl;
#endif
    return realloc(structure, size);
}

extern "C" void mfree(void *structure) {
#ifdef PRINT_MALLOC
    std::cerr << "Freeing" << std::endl;
#endif
    free(structure);
}

extern "C" void mdelete(void *structure) {
#ifdef PRINT_MALLOC
    std::cerr << "Deleting" << std::endl;
#endif
    delete structure;
}

// following split methods are from http://stackoverflow.com/questions/236129/split-a-string-in-c
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
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
     * malloc
     */
    std::vector<llvm::Type *> mmalloc_args;
    mmalloc_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *mmalloc_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                mmalloc_args, false);
    jit->get_module()->getOrInsertFunction("mmalloc", mmalloc_ft);

    /*
     * realloc
     */
    std::vector<llvm::Type *> mrealloc_args;
    mrealloc_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    mrealloc_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    llvm::FunctionType *mrealloc_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                 mrealloc_args, false);
    jit->get_module()->getOrInsertFunction("mrealloc", mrealloc_ft);

    /*
     * free
     */
    std::vector<llvm::Type *> mfree_args;
    mfree_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    llvm::FunctionType *mfree_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                              mfree_args, false);
    jit->get_module()->getOrInsertFunction("mfree", mfree_ft);

    /*
     * delete
     */
    std::vector<llvm::Type *> mdelete_args;
    mdelete_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
    llvm::FunctionType *mdelete_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                           mdelete_args, false);
    jit->get_module()->getOrInsertFunction("mdelete", mdelete_ft);

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