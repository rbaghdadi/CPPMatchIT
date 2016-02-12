//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "./MFunc.h"
#include "./MType.h"

void MFunc::codegen_extern_proto() {
    // set up function parameters
    std::vector<llvm::Type *> param_types;
    for (std::vector<MType*>::iterator iter = extern_param_types.begin(); iter != extern_param_types.end(); iter++) {
//        param_types.push_back(llvm::PointerType::get((*iter)->codegen_type(), 0));
        param_types.push_back((*iter)->codegen_type());
    }
//    param_types.push_back(extern_return_type->codegen_type());
    // add the return type
//    llvm::FunctionType *function_type = llvm::FunctionType::get(extern_return_type->codegen_type(), param_types, false);
    llvm::FunctionType *function_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()), param_types, false);
    // create the function
    extern_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, extern_name,
                                             jit->get_module().get());
}

void MFunc::codegen_extern_wrapper_proto() {
    // set up function parameters
    std::vector<llvm::Type *> param_types;
    for (std::vector<MType *>::iterator iter = extern_wrapper_param_types.begin(); iter != extern_wrapper_param_types.end(); iter++) {
//        param_types.push_back(llvm::PointerType::get(llvm::PointerType::get((*iter)->codegen_type(), 0), 0));
        param_types.push_back((*iter)->codegen_type());
    }
    // also add a int that says how many elements are in the input array
//    param_types.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    // how many total elements in all the data arrays coming in
//    param_types.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    // add the return type
    llvm::FunctionType *function_type = llvm::FunctionType::get(extern_wrapper_return_type->codegen_type(),
                                                                param_types, false);
    // create the function
    extern_wrapper_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                                                     extern_wrapper_name, jit->get_module().get());

}

void MFunc::dump() {
    std::cerr << "Function name: " << extern_name << std::endl;
    std::cerr << "Return type: " << extern_return_type->get_mtype_code() << std::endl;
    std::cerr << "Parameter types: " << std::endl;
    for (std::vector<MType*>::iterator iter = extern_param_types.begin(); iter != extern_param_types.end(); iter++) {
        std::cerr << ((*iter)->get_mtype_code()) << std::endl;
    }
}

void MFunc::verify_wrapper() {
    llvm::verifyFunction(*extern_wrapper_function);
}

const std::string &MFunc::get_extern_name() const {
    return extern_name;
}

const std::string &MFunc::get_extern_wrapper_name() const {
    return extern_wrapper_name;
}

llvm::Function *MFunc::get_extern() const {
    return extern_function;
}

llvm::Function *MFunc::get_extern_wrapper() const {
    return extern_wrapper_function;
}

std::vector<MType *> MFunc::get_extern_param_types() const {
    return extern_param_types;
}

std::vector<MType *> MFunc::get_extern_wrapper_param_types() const {
    return extern_wrapper_param_types;
}

MType *MFunc::get_extern_return_type() const {
    return extern_return_type;
}

MType *MFunc::get_extern_wrapper_return_type() const {
    return extern_wrapper_return_type;
}

const std::string MFunc::get_associated_block() const {
    return associated_block;
}
