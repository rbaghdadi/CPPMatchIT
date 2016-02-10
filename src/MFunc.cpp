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
        param_types.push_back(llvm::PointerType::get((*iter)->codegen(), 0));
    }
    // add the return type
//    llvm::FunctionType *function_type = llvm::FunctionType::get(llvm::PointerType::get(extern_return_type->codegen(), 0), param_types, false);
    llvm::FunctionType *function_type = llvm::FunctionType::get(extern_return_type->codegen(), param_types, false);
    // create the function
    extern_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, extern_name,
                                             jit->get_module().get());
}

void MFunc::codegen_extern_wrapper_proto() {
    // set up function parameters
    std::vector<llvm::Type *> param_types;
    for (std::vector<MType *>::iterator iter = extern_param_types.begin(); iter != extern_param_types.end(); iter++) {
        param_types.push_back(llvm::PointerType::get(llvm::PointerType::get((*iter)->codegen(), 0), 0));
    }
    // also add a int that says how many elements are in the input array
    param_types.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    // add the return type
    llvm::FunctionType *function_type = llvm::FunctionType::get(extern_wrapper_return_type->codegen(), param_types,
                                                                false);
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

std::vector<MType *> MFunc::get_param_types() const {
    return extern_param_types;
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

//    std::vector<llvm::Type *> ret_struct_fields;
//    llvm::PointerType *llvm_ret_type; // need to figure out the appropriate ret type for the block (since some blocks just feed through the original data, but
// have different return types--like Filter with bool return type)
//    if (associated_block.compare("TransformStage") == 0 || associated_block.compare("ComparisonStage") == 0) {
//        llvm_ret_type = llvm::PointerType::get(extern_return_type->codegen(), 0);
//        extern_wrapper_return_type = extern_return_type;
//    } else if (associated_block.compare("FilterStage") == 0) {
//        llvm_ret_type = llvm::PointerType::get(extern_param_types[0]->codegen(), 0);
//        extern_wrapper_return_type = extern_param_types[0];
//    } else if (associated_block.compare("SegmentationStage") == 0) {
//         TODO AHHHHH
//        extern_wrapper_data_ret_type = ((MStructType*)((MPointerType*)((MPointerType*)((MStructType*)((MPointerType*)extern_ret_type)->get_underlying_type())->get_field_types()[0])->get_underlying_type())->get_underlying_type())->get_field_types()[0];
//        extern_wrapper_return_type = ((MStructType*)((MPointerType*)((MStructType*)((MPointerType*) extern_return_type)->get_underlying_type())->get_field_types()[0])->get_underlying_type())->get_field_types()[0];
//        llvm_ret_type = llvm::cast<llvm::PointerType>(extern_wrapper_return_type->codegen());
//    } else { // This is a merged block or something like that
//        llvm_ret_type = llvm::PointerType::get(extern_return_type->codegen(), 0);
//        extern_wrapper_return_type = extern_return_type;
//    }
//    ret_struct_fields.push_back(llvm_ret_type);
//    ret_struct_fields.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
//    llvm::StructType *ret_struct = llvm::StructType::get(llvm::getGlobalContext(), ret_struct_fields);
//    llvm::PointerType *ptr_to_struct = llvm::PointerType::get(ret_struct, 0);
//    llvm::FunctionType *function_type = llvm::FunctionType::get(ptr_to_struct, param_types, false);
