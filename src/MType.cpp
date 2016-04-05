//
// Created by Jessica Ray on 1/15/16.
//

#include <iostream>
#include <string>
#include <memory>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DerivedTypes.h"
#include "./MType.h"
#include "./CodegenUtils.h"
#include "./ForLoop.h"

/*
 * MType
 */

mtype_code_t MType::get_mtype_code() {
    return mtype_code;
}

unsigned int MType::get_bits() {
    int bits_sum = 0;
    if (!underlying_types.empty()) { // if its empty, its a primitive type
        for (std::vector<MType *>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
            bits_sum += (*iter)->get_bits();
        }
    }
    return bits_sum + this->bits;
}

std::vector<MType *> MType::get_underlying_types() {
    return underlying_types;
}

void MType::add_underlying_type(MType *mtype) {
    underlying_types.push_back(mtype);
}

void MType::set_bits(unsigned int bits) {
    this->bits = bits;
}

bool MType::is_int_type() {
    return mtype_code == mtype_char || mtype_code == mtype_short || mtype_code == mtype_int || mtype_code == mtype_long;
}

bool MType::is_float_type() {
    return mtype_code == mtype_float;
}

bool MType::is_bool_type() {
    return mtype_code == mtype_bool;
}

bool MType::is_double_type() {
    return mtype_code == mtype_double;
}

bool MType::is_void_type() {
    return mtype_code == mtype_void;
}

bool MType::is_prim_type() {
    return is_int_type() || is_float_type() || is_double_type() || is_bool_type() || is_void_type() ;
}

bool MType::is_ptr_type() {
    return mtype_code == mtype_ptr;
}

bool MType::is_struct_type() {
    return mtype_code == mtype_struct || mtype_code == mtype_file || mtype_code == mtype_element ||
           mtype_code == mtype_comparison_element || mtype_code == mtype_segments ||
           mtype_code == mtype_segment;
}

bool MType::is_mtype_marray_type() {
    return mtype_code == mtype_marray;
}

bool MType::is_mtype_file_type() {
    return mtype_code == mtype_file;
}

bool MType::is_mtype_element_type() {
    return mtype_code == mtype_element;
}

bool MType::is_mtype_segmented_element_type() {
    return mtype_code == mtype_segment;
}

bool MType::is_mtype_segments_type() {
    return mtype_code == mtype_segments;
}

bool MType::is_mtype_comparison_element_type() {
    return mtype_code == mtype_comparison_element;
}

bool MType::is_mtype_stage() {
    return mtype_code == mtype_wrapper_output;
}

/*
 * MScalarType
 */

// TODO a lot of this codegen_old stuff can be refactored into a single function


llvm::Type *MScalarType::codegen_type() {
    if (is_void_type()) {
        return llvm::Type::getVoidTy(llvm::getGlobalContext());
    } else if (is_int_type()) {
        return llvm::IntegerType::get(llvm::getGlobalContext(), get_bits());
    } else if (is_bool_type()) {
        return llvm::IntegerType::get(llvm::getGlobalContext(), 1);
    } else if (is_float_type()) {
        return llvm::Type::getFloatTy(llvm::getGlobalContext());
    } else if (is_double_type()) {
        return llvm::Type::getDoubleTy(llvm::getGlobalContext());
    } else {
        return nullptr;
    }
}
void MScalarType::dump() {
    std::cerr << "MScalarType with type code: " << mtype_code << std::endl;
}

/*
 * MPointerType
 */

llvm::Type *MPointerType::codegen_type() {
    return llvm::PointerType::get(underlying_types[0]->codegen_type(), 0);
}

void MPointerType::dump() {
    std::cerr << "MPointerType pointing to: " << std::endl;
    underlying_types[0]->dump();
}

llvm::Type *create_struct_type(std::vector<MType *> underlying_types) {
    std::vector<llvm::Type*> llvm_types;
    for (std::vector<MType*>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
        llvm_types.push_back((*iter)->codegen_type());
    }
    llvm::ArrayRef<llvm::Type*> struct_fields(llvm_types);
    return llvm::StructType::get(llvm::getGlobalContext(), struct_fields);
}

/*
 * MStructType
 */

llvm::Type *MStructType::codegen_type() {
    return create_struct_type(underlying_types);
}

void MStructType::dump() {
    int ctr = 0;
    for (std::vector<MType*>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
        std::cerr << "Field " << ctr++ << " has type: ";
        (*iter)->dump();
    }
}

/*
 * Uniqued types
 */

MScalarType *MScalarType::bool_type = new MScalarType(mtype_bool, 1);
MScalarType *MScalarType::char_type = new MScalarType(mtype_char, sizeof(char) * 8);
MScalarType *MScalarType::short_type = new MScalarType(mtype_short, sizeof(short) * 8);
MScalarType *MScalarType::int_type = new MScalarType(mtype_int, sizeof(int) * 8);
MScalarType *MScalarType::long_type = new MScalarType(mtype_long, sizeof(long) * 8);
MScalarType *MScalarType::float_type = new MScalarType(mtype_float, sizeof(float) * 8);
MScalarType *MScalarType::double_type = new MScalarType(mtype_double, sizeof(double) * 8);
MScalarType *MScalarType::void_type = new MScalarType(mtype_void, 0);

MScalarType *MScalarType::get_bool_type() {
    return bool_type;
}

MScalarType *MScalarType::get_char_type() {
    return char_type;
}

MScalarType *MScalarType::get_short_type() {
    return short_type;
}

MScalarType *MScalarType::get_int_type() {
    return int_type;
}

MScalarType *MScalarType::get_long_type() {
    return long_type;
}

MScalarType *MScalarType::get_float_type() {
    return float_type;
}

MScalarType *MScalarType::get_double_type() {
    return double_type;
}

MScalarType *MScalarType::get_void_type() {
    return void_type;
}
