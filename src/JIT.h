//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_JIT_H
#define MATCHIT_JIT_H

#include  <iostream>
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "./Utils.h"

typedef llvm::orc::ObjectLinkingLayer<> ObjLayer;
typedef llvm::orc::IRCompileLayer<ObjLayer> CompileLayer;
typedef CompileLayer::ModuleSetHandleT ModuleHandle;

// TODO make a single instance of this because we shouldn't need multiple versions, and it's a main to keep passing around.
// just make everything static and create an init() function that mimics the constructor

class JIT {

    // TODO global context vs module context?

private:
    // copied from Kaleidoscope tutorial
    std::unique_ptr<llvm::TargetMachine> target_machine;
    llvm::DataLayout data_layout;
    ObjLayer object_layer;
    CompileLayer compile_layer;
    std::vector<ModuleHandle> module_handles;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder; // our builder for the main module

public:

    template <typename T>
    static std::vector<T> singleton(T t) {
        std::vector<T> vec;
        vec.push_back(std::move(t));
        return vec;
    }

    JIT() : target_machine(llvm::EngineBuilder().selectTarget()), data_layout(target_machine->createDataLayout()),
            compile_layer(object_layer, llvm::orc::SimpleCompiler(*target_machine)),
            module(llvm::make_unique<llvm::Module>("the_jit", llvm::getGlobalContext())),
            builder(llvm::getGlobalContext()) {
        llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr); // needed for resolving externs
        module->setDataLayout((*target_machine).createDataLayout());

        // add some useful function wrappers
        // TODO move this somewhere else so the constructor isn't so big

        std::vector<llvm::Type *> llvm_intr_memcpy_args;
        llvm_intr_memcpy_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
        llvm_intr_memcpy_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
        llvm_intr_memcpy_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
        llvm_intr_memcpy_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
        llvm_intr_memcpy_args.push_back(llvm::Type::getInt1Ty(llvm::getGlobalContext()));
        llvm::FunctionType *llvm_intr_memcpy_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                          llvm_intr_memcpy_args, false);
        module->getOrInsertFunction("llvm.memcpy.p0i8.p0i8.i32", llvm_intr_memcpy_ft);

        // llvm is very strict, so we can't use the same malloc call for both 32 and 64 bit data
        std::vector<llvm::Type *> c_malloc32_args;
        c_malloc32_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
        llvm::FunctionType *c_malloc32_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                    c_malloc32_args, false);
        module->getOrInsertFunction("malloc_32", c_malloc32_ft);

        std::vector<llvm::Type *> c_malloc64_args;
        c_malloc64_args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
        llvm::FunctionType *c_malloc64_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                    c_malloc64_args, false);
        module->getOrInsertFunction("malloc_64", c_malloc64_ft);

        // llvm is very strict, so we can't use the same realloc call for both 32 and 64 bit data
        std::vector<llvm::Type *> c_realloc32_args;
        c_realloc32_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
        c_realloc32_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
        llvm::FunctionType *c_realloc32_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                    c_realloc32_args, false);
        module->getOrInsertFunction("realloc_32", c_realloc32_ft);

        std::vector<llvm::Type *> c_realloc64_args;
        c_realloc64_args.push_back(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
        c_realloc64_args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
        llvm::FunctionType *c_realloc64_ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm::getGlobalContext()),
                                                                    c_realloc64_args, false);
        module->getOrInsertFunction("realloc_64", c_realloc64_ft);

        std::vector<llvm::Type *> c_fprintf_args;
        c_fprintf_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
        llvm::FunctionType *c_fprintf_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()), c_fprintf_args, true);
        module->getOrInsertFunction("print_int", c_fprintf_ft);

        llvm::FunctionType *print_sep_ft = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()), std::vector<llvm::Type *>(), false);
        module->getOrInsertFunction("print_sep", print_sep_ft);

    }

    // TODO any other parts need to be cleaned up?
    ~JIT() {
        for (auto handle : llvm::make_range(module_handles.rbegin(), module_handles.rend())) {
            compile_layer.removeModuleSet(handle);
        }
        module_handles.clear();
        std::vector<ModuleHandle>().swap(module_handles); // realloc to size 0
    }

    // TODO these use to have const before and after but too many things needed changing requiring me to get rid of const
    // return by reference, basically like returning a pointer, but you can use it without dereferencing it
    // const in reference return type: can't change the value at that location when returned
    // const at end of declaration: function not allowed to change any class members unless they are marked mutable
    // basically, we return something without having to copy it, which I believe in C would mean you could then
    // modify it. But, with all of this const, you don't copy AND it can't be modified.

    std::unique_ptr<llvm::TargetMachine> &get_target_machine();

    llvm::DataLayout &get_data_layout();

    ObjLayer &get_object_layer();

    CompileLayer &get_compile_layer();

    std::vector<ModuleHandle> &get_module_handles();

    std::unique_ptr<llvm::Module> &get_module();

    llvm::IRBuilder<> &get_builder();

    void dump();

    // Why do these have mangled in the name? I don't think they are doing name mangling in the C++ sense
    std::string mangle(const std::string &name);

    llvm::orc::JITSymbol find_mangled_name(const std::string name);

    // once adding a module, you need to create a new one
    void add_module(std::string jit_name = "jit");

    void run(const std::string func_to_run, const void **data);

};


// DEBUG
//        setbuf(stdout, NULL);
//        std::vector<llvm::Value *> _gep_field_idx;
//        _gep_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        _gep_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        llvm::Module *mod = jit->get_module().get();
//        llvm::GlobalVariable *gv = new llvm::GlobalVariable(*mod, llvm::ArrayType::get(llvm::Type::getInt8Ty(llvm::getGlobalContext()), 3), true, llvm::GlobalValue::ExternalLinkage, nullptr);
//        std::vector<uint8_t> wtf;
//        wtf.push_back('%');
//        wtf.push_back('d');
//        wtf.push_back('\n');
//        llvm::ArrayRef<uint8_t> fuck(wtf);
//        llvm::ConstantDataArray::get(llvm::getGlobalContext(), fuck)->getType()->dump();
//        gv->setInitializer(llvm::ConstantDataArray::get(llvm::getGlobalContext(), fuck));
//        llvm::Function *c_printf = jit->get_module()->getFunction("printf");
//        assert(c_printf);
//        llvm::Value *g = jit->get_builder().CreateInBoundsGEP(gv, _gep_field_idx);
//        std::vector<llvm::Value *> printf_args;
//        printf_args.push_back(g);
////        llvm::Value *bitcast = jit->get_builder().CreateBitCast(ret_idx, llvm::Type::getInt8Ty(llvm::getGlobalContext()));
////        llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), ret_idx);
//        printf_args.push_back(ret_idx);
//        jit->get_builder().CreateCall(c_printf, printf_args);
// END DEBUG

#endif //MATCHIT_JIT_H