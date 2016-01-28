//
// Created by Jessica Ray on 1/28/16.
//

#include "CodegenUtils.h"
#include "./MFunc.h"

void LLVMIR::branch(JIT *jit, llvm::BasicBlock *branch_to) {
    jit->get_builder().CreateBr(branch_to);
}

void LLVMIR::branch_cond(JIT *jit, llvm::Value *condition, llvm::BasicBlock *true_branch,
                         llvm::BasicBlock *false_branch) {
    jit->get_builder().CreateCondBr(condition, true_branch, false_branch);
}

llvm::AllocaInst *LLVMIR::init_loop_idx(JIT *jit, int start_idx) {
    // memory for holding the loop counter
    llvm::AllocaInst *alloc_loop_ctr = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()), nullptr, "loop_idx_alloc");
    alloc_loop_ctr->setAlignment(8);
    llvm::StoreInst *store_init_ctr = jit->get_builder().CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), start_idx),
                                                                     alloc_loop_ctr);
    store_init_ctr->setAlignment(8);
    return alloc_loop_ctr;
}

// TODO don't need start_idx here
void LLVMIR::build_loop_inc_block(JIT *jit, llvm::AllocaInst *alloc, int start_idx, int step_size) {
    // increment the counter
    llvm::LoadInst *load = jit->get_builder().CreateLoad(alloc, "prev_loop_idx");
    load->setAlignment(8);
    llvm::Value *add = jit->get_builder().CreateAdd(load, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), step_size), "next_loop_idx");
    llvm::StoreInst *store = jit->get_builder().CreateStore(add, alloc);
    store->setAlignment(8);
}

llvm::Value *LLVMIR::build_loop_cond_block(JIT *jit, llvm::AllocaInst *alloc_cur_loop_idx, llvm::AllocaInst *max_bound) {
    llvm::LoadInst *load_for_cond = jit->get_builder().CreateLoad(alloc_cur_loop_idx, "cur");
    load_for_cond->setAlignment(8);
    llvm::LoadInst *load_max_bound = jit->get_builder().CreateLoad(max_bound, "end_bound");
    load_max_bound->setAlignment(8);
    llvm::Value *cmp_for_cond = jit->get_builder().CreateICmpSLT(load_for_cond, load_max_bound, "cmp");
    return cmp_for_cond;
}

llvm::Value *LLVMIR::zero32 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0);
llvm::Value *LLVMIR::zero64 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0);

llvm::Value *LLVMIR::one32 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1);
llvm::Value *LLVMIR::one64 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1);

void LLVMIR::codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src) {
    llvm::Function *llvm_memcpy = jit->get_module()->getFunction("llvm.memcpy.p0i8.p0i8.i32");
    assert(llvm_memcpy);
    std::vector<llvm::Value *> memcpy_args;
    memcpy_args.push_back(jit->get_builder().CreateBitCast(dest, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    memcpy_args.push_back(jit->get_builder().CreateBitCast(src, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    // TODO shouldn't just be 400...
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 400));
    // TODO what is the correct alignment for this?
    memcpy_args.push_back(LLVMIR::one32);
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), false));
    jit->get_builder().CreateCall(llvm_memcpy, memcpy_args);
}

void LLVMIR::codegen_ret_idx_inc(JIT *jit, llvm::AllocaInst *alloc_ret_idx, int step_size) {
    llvm::LoadInst *ret_idx = jit->get_builder().CreateLoad(alloc_ret_idx);
    ret_idx->setAlignment(8);
    llvm::Value *add = jit->get_builder().CreateAdd(ret_idx, LLVMIR::one64);
    llvm::StoreInst *store_add = jit->get_builder().CreateStore(add, alloc_ret_idx);
    store_add->setAlignment(8);
}

llvm::Value *LLVMIR::codegen_c_malloc(JIT *jit, size_t size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), size));
    llvm::Value *malloc_field = jit->get_builder().CreateCall(c_malloc, malloc_args);
    return malloc_field;
}

llvm::Value *LLVMIR::codegen_c_malloc_and_cast(JIT *jit, size_t size, llvm::Type *cast_to) {
    llvm::Value *bitcast = jit->get_builder().CreateBitCast(codegen_c_malloc(jit, size), cast_to);
}

llvm::LoadInst *LLVMIR::codegen_load_struct_field(JIT *jit, llvm::AllocaInst *struct_alloc, int field_idx) {
    llvm::LoadInst *loaded_stack_struct = jit->get_builder().CreateLoad(struct_alloc);
    // get the correct index to the data field of the struct (as opposed to the ret_idx field)
    std::vector<llvm::Value *> gep_field_idx;
    gep_field_idx.push_back(LLVMIR::zero32);
    gep_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), field_idx));
    llvm::Value *gep_field = jit->get_builder().CreateInBoundsGEP(loaded_stack_struct, gep_field_idx);
    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(gep_field);
    return loaded_field;
}