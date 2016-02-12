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

typedef llvm::orc::ObjectLinkingLayer<> ObjLayer;
typedef llvm::orc::IRCompileLayer<ObjLayer> CompileLayer;
typedef CompileLayer::ModuleSetHandleT ModuleHandle;

class JIT {

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
    }

    // TODO any other parts need to be cleaned up?
    ~JIT() {
        for (auto handle : llvm::make_range(module_handles.rbegin(), module_handles.rend())) {
            compile_layer.removeModuleSet(handle);
        }
        module_handles.clear();
        std::vector<ModuleHandle>().swap(module_handles); // realloc to size 0
    }

    std::unique_ptr<llvm::TargetMachine> &get_target_machine();

    llvm::DataLayout &get_data_layout();

    ObjLayer &get_object_layer();

    CompileLayer &get_compile_layer();

    std::vector<ModuleHandle> &get_module_handles();

    std::unique_ptr<llvm::Module> &get_module();

    llvm::IRBuilder<> &get_builder();

    void dump();

    std::string mangle(const std::string &name);

    llvm::orc::JITSymbol find_mangled_name(const std::string name);

    // once adding a module, you need to create a new one
    void add_module(std::string jit_name = "jit");

    /**
     * JIT the code and run data through the pipeline.
     */
    void run(const std::string func_to_run, const void **data);

    void run(const std::string func_to_run, const void **data, long total_bytes_in_arrays, long total_elements);


};

#endif //MATCHIT_JIT_H