//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "./MFunc.h"
#include "./MType.h"

// TODO something funky is happening with the arg type iterators in some cases. Maybe change them to regular for loops?
// I think it might be something with my types

void MFunc::codegen_extern_proto() {
    // set up function arguments
    std::vector<llvm::Type *> arg_types;
    for (std::vector<MType*>::iterator iter = extern_arg_types.begin(); iter != extern_arg_types.end(); iter++) {
        arg_types.push_back((*iter)->codegen());
    }
    // add the return type
    llvm::FunctionType *function_type = llvm::FunctionType::get(extern_ret_type->codegen(), arg_types, false);
    // create the function
    extern_func = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, extern_name, jit->get_module().get());
}

void MFunc::codegen_extern_wrapper_proto() {
    // set up function arguments
    std::vector<llvm::Type *> arg_types;
    for (std::vector<MType*>::iterator iter = extern_arg_types.begin(); iter != extern_arg_types.end(); iter++) {
        arg_types.push_back(llvm::PointerType::get((*iter)->codegen(), 0));
    }
    // the actual number of array elements being passed into the function
    arg_types.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    // add the return type
    std::vector<llvm::Type *> ret_struct_fields;
    llvm::PointerType *llvm_ret_type; // need to figure out the appropriate ret type for the block (since some blocks just feed through the original data, but
    // have different return types--like Filter with bool return type)
    if (associated_block.compare("TransformStage") == 0 || associated_block.compare("ComparisonBlock") == 0) {
        llvm_ret_type = llvm::PointerType::get(extern_ret_type->codegen(), 0);
        extern_wrapper_data_ret_type = extern_ret_type;
    } else if (associated_block.compare("FilterStage") == 0) {
        llvm_ret_type = llvm::PointerType::get(extern_arg_types[0]->codegen(), 0);
        extern_wrapper_data_ret_type = extern_arg_types[0];
    } else { // This is a merged block or something like that
        llvm_ret_type = llvm::PointerType::get(extern_ret_type->codegen(), 0);
        extern_wrapper_data_ret_type = extern_ret_type;
    }
    ret_struct_fields.push_back(llvm_ret_type);
    ret_struct_fields.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    llvm::StructType *ret_struct = llvm::StructType::get(llvm::getGlobalContext(), ret_struct_fields);
    llvm::PointerType *ptr_to_struct = llvm::PointerType::get(ret_struct, 0);
    llvm::FunctionType *function_type = llvm::FunctionType::get(ptr_to_struct, arg_types, false);

    // create the function
    extern_wrapper = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, extern_wrapper_name, jit->get_module().get());

}

void MFunc::dump() {
    std::cerr << "Function name: " << extern_name << std::endl;
    std::cerr << "Return type: " << extern_ret_type->get_type_code() << std::endl;
    std::cerr << "Arg types: " << std::endl;
    for (std::vector<MType*>::iterator iter = extern_arg_types.begin(); iter != extern_arg_types.end(); iter++) {
        std::cerr << ((*iter)->get_type_code()) << std::endl;
    }
}

void MFunc::verify_wrapper() {
    llvm::verifyFunction(*extern_wrapper);
}

const std::string &MFunc::get_extern_name() const {
    return extern_name;
}

const std::string &MFunc::get_extern_wrapper_name() const {
    return extern_wrapper_name;
}

llvm::Function *MFunc::get_extern() const {
    return extern_func;
}

llvm::Function *MFunc::get_extern_wrapper() const {
    return extern_wrapper;
}

std::vector<MType *> MFunc::get_arg_types() const {
    return extern_arg_types;
}

MType *MFunc::get_ret_type() const {
    return extern_ret_type;
}

MType *MFunc::get_extern_wrapper_data_ret_type() const {
    return extern_wrapper_data_ret_type;
}