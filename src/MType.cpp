//
// Created by Jessica Ray on 1/15/16.
//

#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DerivedTypes.h"
#include "./MType.h"

mtype_code_t MType::get_type_code() {
    return type_code;
}

unsigned int MType::get_bits() {
    return bits;
}

unsigned int MType::get_alignment() {
    return bits / 8;
}

void MType::set_bits(unsigned int bits) {
    this->bits = bits;
}

bool MType::is_int_type() {
    switch (type_code) {
        case mtype_char:
            return true;
        case mtype_short:
            return true;
        case mtype_int:
            return true;
        case mtype_long:
            return true;
        default:
            return false;
    }
}

bool MType::is_float_type() {
    return type_code == mtype_float;
}

bool MType::is_bool_type() {
    return type_code == mtype_bool;
}

bool MType::is_double_type() {
    return type_code == mtype_double;
}

bool MType::is_prim_type() {
    return is_int_type() || is_float_type() || is_double_type() || is_bool_type();
}

bool MType::is_ptr_type() {
    return type_code == mtype_ptr;
}

bool MType::is_struct_type() {
    return type_code == mtype_struct || type_code == mtype_file || type_code == mtype_element || type_code == mtype_comparison_element || type_code == mtype_segments
            || type_code == mtype_segmented_element;
}

MType *MType::bool_type = new MPrimType(mtype_bool, 1);
MType *MType::char_type = new MPrimType(mtype_char, 8);
MType *MType::short_type = new MPrimType(mtype_short, 16);
MType *MType::int_type = new MPrimType(mtype_int, 32);
MType *MType::long_type = new MPrimType(mtype_long, 64);
MType *MType::float_type = new MPrimType(mtype_float, 32);
MType *MType::double_type = new MPrimType(mtype_double, 64);


/*
 * MPrimType
 */

// TODO a lot of this codegen stuff can be refactored into a single function

unsigned int MPrimType::get_alignment() {
    return MType::get_alignment();
}

llvm::Type *MPrimType::codegen() {
    if (is_int_type()) {
        return llvm::IntegerType::get(llvm::getGlobalContext(), get_bits());
    } else if (is_bool_type()) {
        return llvm::IntegerType::get(llvm::getGlobalContext(), get_bits());
    } else if (is_float_type()) {
        return llvm::Type::getFloatTy(llvm::getGlobalContext());
    } else if (is_double_type()) {
        return llvm::Type::getDoubleTy(llvm::getGlobalContext());
    } else {
        return nullptr;
    }
}

/*
 * MStructType
 */

// bytes
unsigned int MStructType::get_alignment() {
    unsigned int max = 0;
    for (std::vector<MType*>::iterator iter = field_types.begin(); iter != field_types.end(); iter++) {
        if ((*iter)->get_alignment() / 8 > max) {
            max = (*iter)->get_alignment() / 8;
        }
    }
    return max;
}

llvm::Type *MStructType::codegen() {
    std::vector<llvm::Type*> llvm_types;
    for (std::vector<MType*>::iterator iter = field_types.begin(); iter != field_types.end(); iter++) {
        llvm_types.push_back((*iter)->codegen());
    }
    llvm::ArrayRef<llvm::Type*> struct_fields(llvm_types);
    return llvm::StructType::get(llvm::getGlobalContext(), struct_fields);
}

/*
 * MPointerType
 */

unsigned int MPointerType::get_alignment() {
    return MType::get_alignment();
}

llvm::Type *MPointerType::codegen() {
    return llvm::PointerType::get(pointer_type->codegen(), 0);
}