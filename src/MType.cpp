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

// TODO a lot of this codegen stuff can be refactored into a single function


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
 * ElementType
 */

void ElementType::dump() {
    std::cerr << "ElementType has tag type ";
    underlying_types[0]->dump();
    std::cerr << " and x_dimension type ";
    underlying_types[1]->dump();
    std::cerr << " and data type ";
    underlying_types[2]->dump();
}

/*
 * SegmentType
 */

void SegmentType::dump() {
    std::cerr << "SegmentType has tag type ";
    underlying_types[0]->dump();
    std::cerr << " and x_dimension type ";
    underlying_types[1]->dump();
    std::cerr << " and offset type ";
    underlying_types[2]->dump();
    std::cerr << " and data type ";
    underlying_types[3]->dump();
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
ElementType *ElementType::float_element_type = new ElementType(MScalarType::get_float_type());
ElementType *ElementType::uc_element_type = new ElementType(MScalarType::get_char_type());
SegmentType *SegmentType::float_segment_type = new SegmentType(MScalarType::get_float_type());

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

ElementType *ElementType::get_float_element_type() {
    return float_element_type;
}

ElementType *ElementType::get_uc_element_type() {
    return float_element_type;
}

SegmentType *SegmentType::get_float_segment_type() {
    return float_segment_type;
}

/*
 * Preallocation stuff
 */

//// wrapper for creating llvm::AllocaInst and then filling it with malloc'd space
//llvm::AllocaInst *allocator(JIT *jit, llvm::Type *alloca_type, llvm::Value *size_to_malloc, std::string name = "") {
//    llvm::AllocaInst *allocated_space = jit->get_builder().CreateAlloca(alloca_type, nullptr, name);
//    llvm::Value *space = CodegenUtils::codegen_c_malloc64_and_cast(jit, size_to_malloc, alloca_type);
//    jit->get_builder().CreateStore(space, allocated_space);
//    return allocated_space;
//}
//
//// take the preallocated space and combine it.
//// ex:
//// Element<T> **ee = (Element<T>**)malloc(sizeof(Element<T>*) * num_elements);
//// Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//// T *t = (T*)malloc(sizeof(T) * num_prim_values)
//// e needs to be divided up evenly into chunks and placed at ee[0], ee[1],...
//// t needs to be divided up (either fixed, matched, or variable) and placed at e[0], e[1],...
//void divide_preallocated_struct_space(JIT *jit, llvm::AllocaInst *preallocated_struct_ptr_ptr_space,
//                                      llvm::AllocaInst *preallocated_struct_ptr_space,
//                                      llvm::AllocaInst *preallocated_T_ptr_space, llvm::AllocaInst *loop_idx,
//                                      llvm::Value *T_ptr_idx, MType *mtype) {
//    llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
//    // e_ptr_ptr[i] = &e_ptr[i];
//    // gep e_ptr[i]
//    std::vector<llvm::Value *> preallocated_struct_ptr_gep_idx;
//    preallocated_struct_ptr_gep_idx.push_back(cur_loop_idx);
//    llvm::LoadInst *preallocated_struct_ptr_space_load = jit->get_builder().CreateLoad(preallocated_struct_ptr_space);
//    llvm::Value *preallocate_struct_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_space_load,
//                                                                                   preallocated_struct_ptr_gep_idx);
//    // gep e_ptr_ptr[i]
//    std::vector<llvm::Value *> preallocated_struct_ptr_ptr_gep_idx;
//    preallocated_struct_ptr_ptr_gep_idx.push_back(cur_loop_idx);
//    llvm::LoadInst *preallocated_struct_ptr_ptr_space_load =
//            jit->get_builder().CreateLoad(preallocated_struct_ptr_ptr_space);
//    llvm::Value *preallocate_struct_ptr_ptr_gep =
//            jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_ptr_space_load, preallocated_struct_ptr_ptr_gep_idx);
//    jit->get_builder().CreateStore(preallocate_struct_ptr_gep, preallocate_struct_ptr_ptr_gep);
//
//    //  e_ptr[i].data = &t[T_ptr_idx];
//    std::vector<llvm::Value *> struct_ptr_data_gep_idxs;
//    struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(0));
//    // TODO this won't work for types like comparison b/c that has 2 arrays (I think?) Or can I just keep it at 1 array?
//    struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(mtype->get_underlying_types().size() - 1)); // get the last field which contains the data array
//    llvm::Value *struct_ptr_data_gep = jit->get_builder().CreateInBoundsGEP(preallocate_struct_ptr_gep,
//                                                                            struct_ptr_data_gep_idxs);
//    // get t[T_ptr_idx]
//    llvm::LoadInst *preallocated_T_ptr_space_load = jit->get_builder().CreateLoad(preallocated_T_ptr_space);
//    std::vector<llvm::Value *> T_ptr_gep_idx;
//    T_ptr_gep_idx.push_back(T_ptr_idx);
//    llvm::Value *T_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_T_ptr_space_load, T_ptr_gep_idx);
//    jit->get_builder().CreateStore(T_ptr_gep, struct_ptr_data_gep);
//}
//
//// there are always at least two counters that will be generated: the loop index and the loop bound.
//// In the return value, the loop index is at [0], and the loop bound is at [1]
//// if num_loop_counters > 2, then it is up to the calling function to determine what those are used for.
//void preallocate_loop(JIT *jit, ForLoop *loop, llvm::Value *num_structs, llvm::Function *extern_wrapper_function,
//                      llvm::BasicBlock *loop_body, llvm::BasicBlock *dummy) {
//    jit->get_builder().CreateBr(loop->get_counter_bb());
//
//    // counters
//    jit->get_builder().SetInsertPoint(loop->get_counter_bb());
//    // TODO move to loop counters
//    llvm::AllocaInst *loop_bound = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
//    jit->get_builder().CreateStore(num_structs, loop_bound);
//    loop->codegen_counters(loop_bound);
//    jit->get_builder().CreateBr(loop->get_condition_bb());
//
//    // comparison
//    loop->codegen_condition();
//    jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(), loop_body, dummy);
//
//    // loop increment
//    loop->codegen_loop_idx_increment();
//    jit->get_builder().CreateBr(loop->get_condition_bb());
//}
//
//llvm::AllocaInst *MType::preallocate_matched_block(JIT *jit, long num_structs, long num_prim_values, llvm::Function *function,
//                                                         llvm::AllocaInst *input_structs, bool allocate_outer_only) {
//    preallocate_matched_block(jit, CodegenUtils::get_i64(num_structs), CodegenUtils::get_i64(num_prim_values), function,
//                              input_structs, allocate_outer_only);
//}
//
//llvm::AllocaInst *MType::preallocate_matched_block(JIT *jit, llvm::Value *num_structs, llvm::Value *num_prim_values,
//                                                         llvm::Function *function, llvm::AllocaInst *input_structs,
//                                                         bool allocate_outer_only) { // input_structs are the inputs fed into the stage (the {i64, i64, T*}** part)
//    // this is like doing Element<T> **ee = (Element<T>**)malloc(sizeof(Element<T>*) * num_elements);
//    llvm::AllocaInst *preallocated_struct_ptr_ptr_space = allocator(jit, llvm::PointerType::get(llvm::PointerType::get(this->codegen_type(), 0), 0),
//                                                                    jit->get_builder().CreateMul(num_structs, CodegenUtils::get_i64(this->_sizeof_ptr())), "struct_ptr_ptr_pool");
//    if (!allocate_outer_only) { // allocate space for a bunch of new objects. Otherwise, we only allocated space for pointers to new objects (for the FilterStage)
//        // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//        llvm::AllocaInst *preallocated_struct_ptr_space = allocator(jit,
//                                                                    llvm::PointerType::get(this->codegen_type(), 0),
//                                                                    jit->get_builder().CreateMul(num_structs,
//                                                                                                 CodegenUtils::get_i64(
//                                                                                                         this->_sizeof())),
//                                                                    "struct_ptr_pool");
//        // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
//        llvm::AllocaInst *preallocated_T_ptr_space = allocator(jit, llvm::PointerType::get(
//                                                                       this->get_user_type()->codegen_type(), 0),
//                                                               jit->get_builder().CreateMul(num_prim_values,
//                                                                                            CodegenUtils::get_i64(
//                                                                                                    this->_sizeof_T_type())),
//                                                               "prim_ptr_pool");
//
//        // now take the memory pools and split it up across num_elements, but not in a fixed way
//        ForLoop loop(jit);
//        loop.init_codegen(function);
//        llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_body",
//                                                               function);
//        llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_dummy", function);
//        preallocate_loop(jit, &loop, num_structs, function, loop_body, dummy);
//        llvm::AllocaInst *loop_idx = loop.get_loop_idx();
//        llvm::AllocaInst *T_idx = loop.get_return_idx();
//
//        // loop body
//        // divide up the preallocated space appropriately
//        jit->get_builder().SetInsertPoint(loop_body);
//        llvm::LoadInst *T_ptr_idx = jit->get_builder().CreateLoad(T_idx);
//        divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
//                                         preallocated_T_ptr_space, loop_idx, T_ptr_idx, this);
//
//        // increment T_ptr_idx based on the size of the original input
//        llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
//        std::vector<llvm::Value *> input_struct_gep_idx;
//        input_struct_gep_idx.push_back(cur_loop_idx);
//        llvm::Value *input_struct_gep = jit->get_builder().CreateInBoundsGEP(input_structs,
//                                                                             input_struct_gep_idx); // we have the correct struct
//        llvm::LoadInst *input_struct_load = jit->get_builder().CreateLoad(input_struct_gep);
//        std::vector<llvm::Value *> data_gep_idxs;
//        data_gep_idxs.push_back(CodegenUtils::get_i64(0));
//        llvm::Value *data_ptr_ptr_gep = jit->get_builder().CreateInBoundsGEP(input_struct_load, data_gep_idxs);
//        llvm::LoadInst *data_ptr_ptr_load = jit->get_builder().CreateLoad(data_ptr_ptr_gep);
//        std::vector<llvm::Value *> field_two_gep_idxs; // field one of an Element is a tag
//        field_two_gep_idxs.push_back(CodegenUtils::get_i32(0));
//        field_two_gep_idxs.push_back(CodegenUtils::get_i32(1));
//        llvm::Value *field_two_gep = jit->get_builder().CreateInBoundsGEP(data_ptr_ptr_load, field_two_gep_idxs);
//        llvm::LoadInst *length = jit->get_builder().CreateLoad(
//                field_two_gep); // we have the x_dimension of this struct's array now (i.e. num_prim_values for this single Element)
//        llvm::Value *inc_T_idx = jit->get_builder().CreateAdd(T_ptr_idx, length);
//        jit->get_builder().CreateStore(inc_T_idx, T_idx);
//        jit->get_builder().CreateBr(loop.get_increment_bb());
//        jit->get_builder().SetInsertPoint(dummy);
//    }
//    return preallocated_struct_ptr_ptr_space;
//
//}
//
//llvm::AllocaInst *MType::preallocate_fixed_block(JIT *jit, long num_structs, long num_prim_values,
//                                                       int fixed_data_length, llvm::Function *function) {
//    return preallocate_fixed_block(jit, CodegenUtils::get_i64(num_structs), CodegenUtils::get_i64(num_prim_values),
//                                   CodegenUtils::get_i64(fixed_data_length), function);
//}
//
//llvm::AllocaInst *MType::preallocate_fixed_block(JIT *jit, llvm::Value *num_structs, llvm::Value *num_prim_values,
//                                                       llvm::Value *fixed_data_length, llvm::Function *function) {
//    // first create an llvm location for all of this
//    // this is like doing Element<T> **e = (Element<T>**)malloc(sizeof(Element<T>*) * num_elements);
//    llvm::AllocaInst *preallocated_struct_ptr_ptr_space =
//            allocator(jit, llvm::PointerType::get(llvm::PointerType::get(this->codegen_type(), 0), 0),
//                      jit->get_builder().CreateMul(num_structs, CodegenUtils::get_i64(this->_sizeof_ptr())), "struct_ptr_ptr_pool");
//    // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//    llvm::AllocaInst *preallocated_struct_ptr_space =
//            allocator(jit, llvm::PointerType::get(this->codegen_type(), 0),
//                      jit->get_builder().CreateMul(num_structs, CodegenUtils::get_i64(this->_sizeof())), "struct_ptr_pool");
//    // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
//    llvm::AllocaInst *preallocated_T_ptr_space =
//            allocator(jit, llvm::PointerType::get(this->get_user_type()->codegen_type(), 0),
//                      jit->get_builder().CreateMul(num_prim_values, CodegenUtils::get_i64(this->_sizeof_T_type())), "prim_ptr_pool");
//
//    // now take the memory pools and split it up evenly across num_elements
//    ForLoop loop(jit);
//    loop.init_codegen(function);
//    llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_body", function);
//    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_dummy", function);
//    preallocate_loop(jit, &loop, num_structs, function, loop_body, dummy);
//    llvm::AllocaInst *loop_idx = loop.get_loop_idx();
//
//    // loop body
//    // divide up the preallocated space appropriately
//    jit->get_builder().SetInsertPoint(loop_body);
//    llvm::Value *T_ptr_idx = jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop_idx), fixed_data_length);
//    divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
//                                     preallocated_T_ptr_space, loop_idx, T_ptr_idx, this);
//    jit->get_builder().CreateBr(loop.get_increment_bb());
//
//    jit->get_builder().SetInsertPoint(dummy);
//
//    return preallocated_struct_ptr_ptr_space;
//}