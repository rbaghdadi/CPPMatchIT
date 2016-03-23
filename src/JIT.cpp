//
// Created by Jessica Ray on 1/28/16.
//

#include "llvm/IR/Mangler.h"
#include "llvm/ExecutionEngine/Orc/IndirectionUtils.h"
#include "./JIT.h"

std::unique_ptr<llvm::TargetMachine> &JIT::get_target_machine() {
    return target_machine;
}

llvm::DataLayout &JIT::get_data_layout() {
    return data_layout;
}

ObjLayer &JIT::get_object_layer() {
    return object_layer;
}

CompileLayer &JIT::get_compile_layer() {
    return compile_layer;
}

std::vector<ModuleHandle> &JIT::get_module_handles() {
    return module_handles;
}

std::unique_ptr<llvm::Module> &JIT::get_module() {
    return module;
}

llvm::IRBuilder<> &JIT::get_builder() {
    return builder;
}

void JIT::dump() {
    std::cerr << "=========== DEBUG" << std::endl;
    std::cerr << "=========== MODULE DUMP" << std::endl;
    module->dump();
}

std::string JIT::mangle(const std::string &name) {
    std::string mangled_name;
    {
        llvm::raw_string_ostream mangled_name_stream(mangled_name);
        llvm::Mangler::getNameWithPrefix(mangled_name_stream, name, data_layout);
    }
    return mangled_name;
}

llvm::orc::JITSymbol JIT::find_mangled_name(std::string name) {
    // check our module for the function
    for (auto handle : llvm::make_range(module_handles.rbegin(), module_handles.rend())) {
        if (auto sym = compile_layer.findSymbolIn(handle, name, true)) {
            return sym;
        }
    }
    // fall back to other libraries
    if (auto sym_addr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)) {
        return  llvm::orc::JITSymbol(sym_addr, llvm::JITSymbolFlags::Exported);
    }
    return nullptr;
}

// all from Kaleidoscope basically
// TODO figure out what all this stuff is (like the compiler layer and such)
void JIT::add_module(std::string jit_name) {
//    llvm::orc::makeAllSymbolsExternallyAccessible(*module);
    auto resolver = llvm::orc::createLambdaResolver(
            [&](const std::string &name) {
                if (auto sym = find_mangled_name(name)) {
                    return llvm::RuntimeDyld::SymbolInfo(sym.getAddress(), sym.getFlags());
                }
            },
            [](const std::string &s) {
                return nullptr;
            });

    // allocate memory for the symbol (make a handle)
    auto allocator = compile_layer.addModuleSet(JIT::singleton(std::move(module)),
                                                llvm::make_unique<llvm::SectionMemoryManager>(),
                                                std::move(resolver));

    module_handles.push_back(allocator);
    // make a new module
    module = llvm::make_unique<llvm::Module>(jit_name, llvm::getGlobalContext());
    module->setDataLayout((*target_machine).createDataLayout());
}

//void JIT::add_module(llvm::Module *mod) {
////    llvm::orc::makeAllSymbolsExternallyAccessible(*mod);
//    auto resolver = llvm::orc::createLambdaResolver(
//            [&](const std::string &name) {
//                if (auto sym = find_mangled_name(name)) {
//                    return llvm::RuntimeDyld::SymbolInfo(sym.getAddress(), sym.getFlags());
//                }
//            },
//            [](const std::string &s) {
//                return nullptr;
//            });
//
//    // allocate memory for the symbol (make a handle)
//    auto do_malloc = compile_layer.addModuleSet(JIT::singleton(std::move(mod)),
//                                                llvm::make_unique<llvm::SectionMemoryManager>(),
//                                                std::move(resolver));
//
//    module_handles.push_back(do_malloc);
//    // make a new module
////    module = llvm::make_unique<llvm::Module>(jit_name, llvm::getGlobalContext());
////    module->setDataLayout((*target_machine).createDataLayout());
//}

void JIT::run(const std::string func_to_run, const void **data) {
    auto jit_sym = find_mangled_name(mangle(func_to_run));
    void (*jit_func)(const void **) = (void (*)(const void **))(intptr_t)jit_sym.getAddress();
    jit_func(data);
}

#define jit_args(...) __VA_ARGS__


void JIT::run(const std::string func_to_run, ...) {
    auto jit_sym = find_mangled_name(mangle(func_to_run));
    void (*jit_func)(...) = (void (*)(...))(intptr_t)jit_sym.getAddress();

}

void JIT::run(const std::string func_to_run, const void **in_setelements, int in_num, const void **out_setelements,
              int out_num, const void *base_field1, const void *base_field2) {
    auto jit_sym = find_mangled_name(mangle(func_to_run));
    void (*jit_func)(const void **, int, const void **, int, const void *, const void *) =
            (void (*)(const void **, int, const void **, int, const void *, const void *))(intptr_t)jit_sym.getAddress();
    jit_func(in_setelements, in_num, out_setelements, out_num, base_field1, base_field2);
}

void JIT::run(const std::string func_to_run, const void **data, long total_bytes_in_arrays, long total_elements) {
    auto jit_sym = find_mangled_name(mangle(func_to_run));
    void (*jit_func)(const void **, long, long) = (void (*)(const void **, long, long))(intptr_t)jit_sym.getAddress();
    jit_func(data, total_bytes_in_arrays, total_elements);
}