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
//    else {
//        bits_sum += this->bits;
//    }
    return bits_sum + this->bits;
//    return bits;
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
    return mtype_code == mtype_wrapper_output;
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


llvm::Type *MPrimType::codegen_type() {
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

}

//// if the type is X, do X *x = (X*)malloc(sizeof(X) * num_elements);
//llvm::Value *MStructType::codegen_pool(JIT *jit, int num_elements) {
//    // get the llvm type representing X
//    llvm::Type *llvm_type = llvm::PointerType::get(codegen_type(), 0);
//    // allocate enough space for num_elements worth of this type
//    size_t pool_size = sizeof(*this) * num_elements;
//    // mallocs i8* and casts to X*
//    llvm::Value *pool = CodegenUtils::codegen_c_malloc64_and_cast(jit, pool_size, llvm_type);
//}

/*
 * MArrayType
 */

void MArrayType::dump() {
    std::cerr << "MArrayType data type: ";
    underlying_types[0]->dump();
    std::cerr << " and ";
    underlying_types[1]->dump();
    underlying_types[2]->dump();
}

//llvm::Type *MArrayType::codegen() {
//    return create_struct_type(underlying_types);
//}

/*
 * FileType
 */

void FileType::dump() {
    std::cerr << "FileType has field type ";
    underlying_types[0]->dump();
}

//llvm::Type *FileType::codegen() {
//    return create_struct_type(underlying_types);
//}

/*
 * ElementType
 */

void ElementType::dump() {
    std::cerr << "ElementType has tag type ";
    underlying_types[0]->dump();
    std::cerr << " and length type ";
    underlying_types[1]->dump();
    std::cerr << " and data type ";
    underlying_types[2]->dump();
}

llvm::AllocaInst * ElementType::preallocate_block(JIT *jit, int num_elements, int fixed_data_length,
                                                  llvm::Function *function) {
    return preallocate_block(jit, CodegenUtils::get_i64(num_elements), CodegenUtils::get_i64(fixed_data_length), function);
}

llvm::AllocaInst * ElementType::preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                                  llvm::Function *function) {
    llvm::BasicBlock *preallocate = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate", function);
    jit->get_builder().CreateBr(preallocate);
    jit->get_builder().SetInsertPoint(preallocate);
    // first create an llvm location for all of this
    // this looks like %a = alloca {i32, i32, T*}**
    llvm::AllocaInst *preallocated_ele_ptr_ptr_space = jit->get_builder().CreateAlloca(llvm::PointerType::get(llvm::PointerType::get(this->codegen_type(), 0), 0), nullptr, "element_ptr_ptr_pool");
    llvm::AllocaInst *preallocated_ele_ptr_space = jit->get_builder().CreateAlloca(llvm::PointerType::get(this->codegen_type(), 0), nullptr, "element_ptr_pool");
    llvm::AllocaInst *preallocated_T_ptr_space = jit->get_builder().CreateAlloca(llvm::PointerType::get(this->get_user_type()->codegen_type(), 0), nullptr, "T_ptr_pool");

    // allocate space for the T* across all num_elements
    // this is like doing T *t = (T*)malloc(sizeof(T) * num_elements * fixed_data_length);
    llvm::Value *size_for_T_ptr = jit->get_builder().CreateMul(num_elements, jit->get_builder().CreateMul(fixed_data_length, CodegenUtils::get_i64(this->_sizeof_T_type())));
    llvm::Value *T_ptr = CodegenUtils::codegen_c_malloc64_and_cast(jit, size_for_T_ptr, llvm::PointerType::get(this->get_user_type()->codegen_type(), 0));
    jit->get_builder().CreateStore(T_ptr, preallocated_T_ptr_space);

    // allocate space for {i32, i32, T*}*
    // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
    llvm::Value *size_for_element_ptr = jit->get_builder().CreateMul(num_elements, CodegenUtils::get_i64(this->_sizeof()));
    llvm::Value *element_ptr = CodegenUtils::codegen_c_malloc64_and_cast(jit, size_for_element_ptr, llvm::PointerType::get(this->codegen_type(), 0));
    jit->get_builder().CreateStore(element_ptr, preallocated_ele_ptr_space);

    // allocate space for the whole {i32, i32, T*}**
    // this is like doing Element<T> **e = (Element<T>**)malloc(sizeof(Element<T>*) * num_elements);
    llvm::Value* size_for_element_ptr_ptr = jit->get_builder().CreateMul(num_elements, CodegenUtils::get_i64(this->_sizeof_ptr()));
    llvm::Value *element_ptr_ptr = CodegenUtils::codegen_c_malloc64_and_cast(jit, size_for_element_ptr_ptr, llvm::PointerType::get(llvm::PointerType::get(this->codegen_type(), 0), 0));
    jit->get_builder().CreateStore(element_ptr_ptr, preallocated_ele_ptr_ptr_space);
    // now take the memory pools and split it up across num_elements
    // create the for loop components
    llvm::BasicBlock *loop_counter = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_counters", function);
    llvm::BasicBlock *loop_condition = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_condition", function);
    llvm::BasicBlock *loop_increment = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_increment", function);
    llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_body", function);
    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_dummy", function);
    jit->get_builder().CreateBr(loop_counter);

    // counters
    jit->get_builder().SetInsertPoint(loop_counter);
    llvm::Type *counter_type = llvm::Type::getInt64Ty(llvm::getGlobalContext());
    llvm::AllocaInst *loop_idx = jit->get_builder().CreateAlloca(counter_type);
    jit->get_builder().CreateStore(CodegenUtils::get_i64(0), loop_idx);
    llvm::AllocaInst *loop_bound = jit->get_builder().CreateAlloca(counter_type);
    jit->get_builder().CreateStore(num_elements, loop_bound);
    jit->get_builder().CreateBr(loop_condition);

    // comparison
    jit->get_builder().SetInsertPoint(loop_condition);
    llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
    llvm::LoadInst *bound = jit->get_builder().CreateLoad(loop_bound);
    llvm::Value *cmp = jit->get_builder().CreateICmpSLT(cur_loop_idx, bound);
    jit->get_builder().CreateCondBr(cmp, loop_body, dummy);

    // loop body
    jit->get_builder().SetInsertPoint(loop_body);
    llvm::LoadInst *cur_loop_idx2 = jit->get_builder().CreateLoad(loop_idx);
    // e_ptr_ptr[i] = &e_ptr[i];
    // gep e_ptr[i]
    std::vector<llvm::Value *> preallocated_e_ptr_gep_idx;
    preallocated_e_ptr_gep_idx.push_back(cur_loop_idx2);
    llvm::LoadInst *preallocated_ele_ptr_space_load = jit->get_builder().CreateLoad(preallocated_ele_ptr_space);
    llvm::Value *preallocate_e_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_ele_ptr_space_load, preallocated_e_ptr_gep_idx);
    // gep e_ptr_ptr[i]
    std::vector<llvm::Value *> preallocated_e_ptr_ptr_gep_idx;
    preallocated_e_ptr_ptr_gep_idx.push_back(cur_loop_idx2);
    llvm::LoadInst *preallocated_ele_ptr_ptr_space_load = jit->get_builder().CreateLoad(preallocated_ele_ptr_ptr_space);
    llvm::Value *preallocate_e_ptr_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_ele_ptr_ptr_space_load, preallocated_e_ptr_ptr_gep_idx);
    jit->get_builder().CreateStore(preallocate_e_ptr_gep, preallocate_e_ptr_ptr_gep);

    // e_ptr[i].data = &t[i * fixed_data_length];
    // already have e_ptr[i], get .data
    std::vector<llvm::Value *> e_ptr_data_gep_idxs;
    e_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(0));
    e_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(2));
    llvm::Value *e_ptr_data_gep = jit->get_builder().CreateInBoundsGEP(preallocate_e_ptr_gep,e_ptr_data_gep_idxs);
    // get t[i * fixed_data_length]
    llvm::Value *T_ptr_idx = jit->get_builder().CreateMul(cur_loop_idx2, fixed_data_length);
    llvm::LoadInst *preallocated_T_ptr_space_load = jit->get_builder().CreateLoad(preallocated_T_ptr_space);
    std::vector<llvm::Value *> T_ptr_gep_idx;
    T_ptr_gep_idx.push_back(T_ptr_idx);
    llvm::Value *T_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_T_ptr_space_load, T_ptr_gep_idx);
    jit->get_builder().CreateStore(T_ptr_gep, e_ptr_data_gep);
    jit->get_builder().CreateBr(loop_increment);

    // loop increment
    jit->get_builder().SetInsertPoint(loop_increment);
    llvm::LoadInst *load = jit->get_builder().CreateLoad(loop_idx);
    llvm::Value *inc = jit->get_builder().CreateAdd(load, CodegenUtils::get_i64(1));
    jit->get_builder().CreateStore(inc, loop_idx);
    jit->get_builder().CreateBr(loop_condition);

    jit->get_builder().SetInsertPoint(dummy);
    return preallocated_ele_ptr_ptr_space;
}


//llvm::Type *ElementType::codegen() {
//    return create_struct_type(underlying_types);
//}

/*
 * WrapperOutputType
 */

void WrapperOutputType::dump() {
    std::cerr << "WrapperOutputType has field types ";
    underlying_types[0]->dump();
}

//llvm::Type *WrapperOutputType::codegen() {
//    return create_struct_type(underlying_types);
//}

/*
 * SegmentedElementType
 */

void SegmentedElementType::dump() {
    std::cerr << "SegmentedElementType has field types ";
    underlying_types[0]->dump();
    std::cerr << " and ";
    underlying_types[1]->dump();
    std::cerr << " and ";
    underlying_types[2]->dump();
}

//llvm::Type *SegmentedElementType::codegen() {
//    return create_struct_type(underlying_types);
//}

/*
 * SegmentsType
 */

void SegmentsType::dump() {
    std::cerr << "SegmentsType has field types ";
    underlying_types[0]->dump();
}

//llvm::Type *SegmentsType::codegen() {
//    return create_struct_type(underlying_types);
//}

/*
 * ComparisonElement
 */

void ComparisonElementType::dump() {
    std::cerr << "ComparisonElementType has field types ";
    underlying_types[0]->dump();
    std::cerr << " and ";
    underlying_types[1]->dump();
    std::cerr << " and ";
    underlying_types[2]->dump();
}

//llvm::Type *ComparisonElementType::codegen() {
//    return create_struct_type(underlying_types);
//}

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
