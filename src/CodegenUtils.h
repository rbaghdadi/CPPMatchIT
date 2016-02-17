//
// Created by Jessica Ray on 1/22/16.
//

// A collection of common codegen stuff for LLVM IR

#ifndef MATCHIT_CODEGENUTILS_H
#define MATCHIT_CODEGENUTILS_H

#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"
#include "./MType.h"
#include "./JIT.h"
#include "./MFunc.h"
#include "./InstructionBlock.h"
//
//llvm::Value *codegen_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit);
//llvm::Value *codegen_segmented_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit);
//llvm::Value *codegen_file_size(llvm::AllocaInst *extern_call_res, JIT *jit);
//llvm::Value *codegen_comparison_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit);
//void codegen_file_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
//                        llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
//                        JIT *jit, llvm::Function *insert_into);
//void codegen_element_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
//                           llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
//                           JIT *jit, llvm::Function *insert_into);
//void codegen_comparison_element_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
//                                      llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
//                                      JIT *jit, llvm::Function *insert_into);
//void codegen_segments_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
//                            llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
//                            JIT *jit, llvm::Function *insert_into);


namespace CodegenUtils {
// A component of a function. Can be anything really
// wraps LLVM IR
class FuncComp {
private:
    // where the result of this computation is stored
    std::vector<llvm::Value *> results;
public:
    FuncComp(llvm::Value *result) : results(std::vector<llvm::Value *>()) {
        this->results.push_back(result);
    }

    FuncComp(std::vector<llvm::Value *> results) : results(results) { }

    llvm::Value *get_result() {
        return results[0];
    }

    std::vector<llvm::Value *> get_results() {
        return results;
    }

    llvm::Value *get_result(unsigned int i) {
        return results[i];
    }

    size_t get_size() {
        return results.size();
    }

    llvm::Value *get_last() {
        return results[results.size() - 1];
    }
};

llvm::Value *get_mtype_file_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc);
llvm::Value *get_mtype_element_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *element_type);
llvm::Value *get_mtype_comparison_element_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *comparison_element_type);
llvm::Value *get_mtype_segmented_element_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *segmented_element_type);
llvm::Value *get_mtype_segments_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *segments_type, MFunc *mfunction);


std::vector<llvm::AllocaInst *> load_wrapper_input_args(JIT *jit, llvm::Function *function);

std::vector<llvm::AllocaInst *> load_extern_input_arg(JIT *jit, llvm::AllocaInst *wrapper_input_arg_alloc,
                                                      llvm::AllocaInst *preallocated_output,
                                                      llvm::AllocaInst *loop_idx);

llvm::AllocaInst * create_extern_call(JIT *jit, llvm::Function *extern_function,
                                      std::vector<llvm::AllocaInst *> extern_arg_allocs);

llvm::AllocaInst *init_i64(JIT *jit, int start_val = 0, std::string name = "");

void increment_i64(JIT *jit, llvm::AllocaInst *i64_val, int step_size = 1);

llvm::Value * create_loop_condition_check(JIT *jit, llvm::AllocaInst *loop_idx_alloc, llvm::AllocaInst *max_loop_bound);

llvm::AllocaInst *init_wrapper_output_struct(JIT *jit, MFunc *mfunction, llvm::AllocaInst *max_loop_bound,
                                             llvm::AllocaInst *malloc_size);

void store_result(llvm::AllocaInst *wrapper_output_struct_alloc, JIT *jit, llvm::AllocaInst *output_idx,
                  llvm::AllocaInst *data_to_store, llvm::Function *insert_into, llvm::AllocaInst *malloc_size_alloc,
                  ExternCallStoreIB *extern_call_store_ib, MFunc *mfunction);

void return_data(JIT *jit, llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *output_idx);

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src, llvm::Value *bytes_to_copy);

llvm::Value *codegen_c_malloc32(JIT *jit, size_t size);

llvm::Value *codegen_c_malloc32(JIT *jit, llvm::Value *size);

llvm::Value *codegen_c_malloc64(JIT *jit, size_t size);

llvm::Value *codegen_c_malloc64(JIT *jit, llvm::Value *size);

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, size_t size, llvm::Type *cast_to);

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_realloc(JIT *jit, llvm::Function *c_realloc, llvm::LoadInst *loaded_structure, llvm::Value *size);

llvm::Value *codegen_c_realloc32(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size);

llvm::Value *codegen_c_realloc64(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size);

llvm::Value *codegen_c_realloc32_and_cast(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size, llvm::Type *cast_to);

llvm::Value *codegen_c_realloc64_and_cast(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size, llvm::Type *cast_to);

void codegen_fprintf_int(JIT *jit, llvm::Value *the_int);

void codegen_fprintf_int(JIT *jit, int the_int);

void codegen_fprintf_float(JIT *jit, llvm::Value *the_int);

void codegen_fprintf_float(JIT *jit, float the_int);

// some useful things
llvm::ConstantInt *get_i1(int zero_or_one);

llvm::ConstantInt *get_i32(int x);

llvm::ConstantInt *get_i64(long x);

llvm::Value *gep(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx);
llvm::LoadInst *gep_and_load(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx);

// if the type is X, do X *x = (X*)malloc(sizeof(X) * num_elements);
//llvm::Value *codegen_mtype_block(MType *mtype, JIT *jit, int num_elements) {
//    llvm::Type *llvm_mtype = llvm::PointerType::get(mtype->codegen_type(), 0);
//    // allocate enough space for num_elements worth of mtype
//    size_t pool_size = mtype->_sizeof() * num_elements;
//    // mallocs i8* and casts to X*
//    llvm::Value *pool = CodegenUtils::codegen_c_malloc64_and_cast(jit, pool_size, llvm_mtype);
//    return pool;
//}

int get_num_size_fields(MType *mtype);

}

#endif //MATCHIT_CODEGENUTILS_H
