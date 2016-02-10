//
// Created by Jessica Ray on 1/15/16.
//

#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DerivedTypes.h"
#include "./MType.h"

/*
 * MType
 */

mtype_code_t MType::get_mtype_code() {
    return mtype_code;
}

unsigned int MType::get_bits() {
    return bits;
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
           mtype_code == mtype_segmented_element;
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
    return mtype_code == mtype_segmented_element;
}

bool MType::is_mtype_segments_type() {
    return mtype_code == mtype_segments;
}

bool MType::is_mtype_comparison_element_type() {
    return mtype_code == mtype_comparison_element;
}

bool MType::is_mtype_stage() {
    return mtype_code == mtype_stage;
}

MType *MType::bool_type = new MPrimType(mtype_bool, 1);
MType *MType::char_type = new MPrimType(mtype_char, 8);
MType *MType::short_type = new MPrimType(mtype_short, 16);
MType *MType::int_type = new MPrimType(mtype_int, 32);
MType *MType::long_type = new MPrimType(mtype_long, 64);
MType *MType::float_type = new MPrimType(mtype_float, 32);
MType *MType::double_type = new MPrimType(mtype_double, 64);
MType *MType::void_type = new MPrimType(mtype_void, 0);

MType *MType::get_bool_type() {
    return bool_type;
}

MType *MType::get_char_type() {
    return char_type;
}

MType *MType::get_short_type() {
    return short_type;
}

MType *MType::get_int_type() {
    return int_type;
}

MType *MType::get_long_type() {
    return long_type;
}

MType *MType::get_float_type() {
    return float_type;
}

MType *MType::get_double_type() {
    return double_type;
}

MType *MType::get_void_type() {
    return void_type;
}

/*
 * MPrimType
 */

// TODO a lot of this codegen stuff can be refactored into a single function


llvm::Type *MPrimType::codegen() {
    if (is_int_type()) {
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
void MPrimType::dump() {
    std::cerr << "MPrimType with type code: " << mtype_code << std::endl;
}

/*
 * MPointerType
 */

llvm::Type *MPointerType::codegen() {
    return llvm::PointerType::get(underlying_types[0]->codegen(), 0);
}

void MPointerType::dump() {
    std::cerr << "MPointerType pointing to: " << std::endl;
    underlying_types[0]->dump();
}

llvm::Type *create_struct_type(std::vector<MType *> underlying_types) {
    std::vector<llvm::Type*> llvm_types;
    for (std::vector<MType*>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
        llvm_types.push_back((*iter)->codegen());
    }
    llvm::ArrayRef<llvm::Type*> struct_fields(llvm_types);
    return llvm::StructType::get(llvm::getGlobalContext(), struct_fields);
}

/*
 * MArrayType
 */

void MArrayType::dump() {
    std::cerr << "MArrayType data type: ";
    underlying_types[0]->dump();
    std::cerr << "followed by " << std::endl;
    underlying_types[1]->dump();
    underlying_types[2]->dump();
}

llvm::Type *MArrayType::codegen() {
    return create_struct_type(underlying_types);
}

/*
 * FileType
 */

void FileType::dump() {
    std::cerr << "FileType has field type ";
    underlying_types[0]->dump();
}

llvm::Type *FileType::codegen() {
    return create_struct_type(underlying_types);
}

FileType *create_filetype() {
    return new FileType();
};

/*
 * ElementType
 */

void ElementType::dump() {
    std::cerr << "ElementType has field types ";
    underlying_types[0]->dump();
    std::cerr << " and ";
    underlying_types[1]->dump();
}

llvm::Type *ElementType::codegen() {
    return create_struct_type(underlying_types);
}

/*
 * WrapperOutputType
 */

void WrapperOutputType::dump() {
    std::cerr << "WrapperOutputType has field types ";
    underlying_types[0]->dump();
}

llvm::Type *WrapperOutputType::codegen() {
    return create_struct_type(underlying_types);
}

//int main() {
//    MArrayType *t = create_marraytype<char>();
//    FileType *f = create_filetype();
//    ElementType *e = create_elementtype<float>();
//    t->dump();
//    f->dump();
//    e->dump();
//
//    return 0;
//}

/*
 * MStructType
 */
//
//llvm::Type *MStructType::codegen() {
//    std::vector<llvm::Type*> llvm_types;
//    for (std::vector<MType*>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
//        llvm_types.push_back((*iter)->codegen());
//    }
//    llvm::ArrayRef<llvm::Type*> struct_fields(llvm_types);
//    return llvm::StructType::get(llvm::getGlobalContext(), struct_fields);
//}
//
//void MStructType::dump() {
//    std::cerr << "MStructType with type code: " << mtype_code << " and field types " << std::endl;
//    for (std::vector<MType *>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
//        (*iter)->dump();
//        std::cerr << "###" << std::endl;
//    }
//}
