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

std::vector<MType *> MType::get_underlying_types() {
    return underlying_types;
}

void MType::add_underlying_type(MType *mtype) {
    underlying_types.push_back(mtype);
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

bool MType::is_scalar_type() {
    return is_int_type() || is_float_type() || is_double_type() || is_bool_type() || is_void_type();
}

bool MType::is_signed_type() {
    return mtype_code == mtype_char || mtype_code == mtype_short || mtype_code == mtype_int || mtype_code == mtype_long;
}

bool MType::is_ptr_type() {
    return mtype_code == mtype_ptr;
}

bool MType::is_struct_type() {
    return mtype_code == mtype_struct;
}

bool MType::is_mtype_marray_type() {
    return mtype_code == mtype_marray;
}

bool MType::is_mscalar_type() {
    return false;
}

bool MType::is_mstruct_type() {
    return false;
}

bool MType::is_mpointer_type() {
    return false;
}

bool MType::is_marray_type() {
    return false;
}

bool MType::is_mmatrix_type() {
    return false;
}

/*
 * MScalarType
 */

llvm::Type *MScalarType::codegen_type() {
    if (is_void_type()) {
        return llvm::Type::getVoidTy(llvm::getGlobalContext());
    } else if (is_int_type()) {
        return llvm::IntegerType::get(llvm::getGlobalContext(), underlying_size() * 8);
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

int MScalarType::underlying_size() {
    return bytes;
}

bool MScalarType::is_mscalar_type() {
    return true;
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

int MPointerType::underlying_size() {
    return underlying_types[0]->underlying_size();
}

bool MPointerType::is_mpointer_type() {
    return true;
}

/*
 * MStructType
 */

llvm::Type *create_struct_type(std::vector<MType *> underlying_types) {
    std::vector<llvm::Type*> llvm_types;
    for (std::vector<MType*>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
        llvm_types.push_back((*iter)->codegen_type());
    }
    llvm::ArrayRef<llvm::Type*> struct_fields(llvm_types);
    return llvm::StructType::get(llvm::getGlobalContext(), struct_fields);
}

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

int MStructType::underlying_size() {
    int total = 0;
    for (std::vector<MType *>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
        total += (*iter)->underlying_size();
    }
    return total;
}

bool MStructType::is_mstruct_type() {
    return true;
}

/*
 * MArrayType
 */

void MArrayType::dump() {
    std::cerr << "MArrayType with element type: ";
    array_element_type->dump();
}

MType *MArrayType::get_array_element_type() {
    return array_element_type;
}

int MArrayType::get_length() {
    return length;
}

llvm::Type *MArrayType::codegen_type() {
    assert(false); // this isn't needed currently
    return nullptr;
}

int MArrayType::underlying_size() {
    return array_element_type->underlying_size();
}

bool MArrayType::is_marray_type() {
    return true;
}

/*
 * MMatrixType
 */

int MMatrixType::get_row_dimension() {
    return row_dimension;
}

int MMatrixType::get_col_dimension() {
    return col_dimension;
}

MType *MMatrixType::get_matrix_element_type() {
    return matrix_element_type;
}

void MMatrixType::dump() {
    std::cerr << "MMatrixType with element type: ";
    matrix_element_type->dump();
}

llvm::Type *MMatrixType::codegen_type() {
    assert(false);
    return nullptr;
}

int MMatrixType::underlying_size() {
    return matrix_element_type->underlying_size();
}

bool MMatrixType::is_mmatrix_type() {
    return true;
}

/*
 * Premade MScalarType values
 */

MScalarType *MScalarType::bool_type = new MScalarType(mtype_bool, 1);
MScalarType *MScalarType::char_type = new MScalarType(mtype_char, sizeof(char));
MScalarType *MScalarType::short_type = new MScalarType(mtype_short, sizeof(short));
MScalarType *MScalarType::int_type = new MScalarType(mtype_int, sizeof(int));
MScalarType *MScalarType::long_type = new MScalarType(mtype_long, sizeof(long));
MScalarType *MScalarType::float_type = new MScalarType(mtype_float, sizeof(float));
MScalarType *MScalarType::double_type = new MScalarType(mtype_double, sizeof(double));
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
