//
// Created by Jessica Ray on 1/31/16.
//

#include "./InstructionBlock.h"
#include "./Utils.h"
#include "./CodegenUtils.h"

/*
 * InstructionBlock
 */

llvm::BasicBlock *InstructionBlock::get_basic_block() {
    return bb;
}

MFunc *InstructionBlock::get_mfunction() {
    return mfunction;
}

void InstructionBlock::set_mfunction(MFunc *mfunction) {
    this->mfunction = mfunction;
}

void InstructionBlock::force_insert(MFunc *mfunction) {
    bb->insertInto(mfunction->get_extern_wrapper());
}

/*
 * WrapperArgLoaderIB
 */

std::vector<llvm::AllocaInst *> WrapperArgLoaderIB::get_args_alloc() {
    return args_alloc;
}

void WrapperArgLoaderIB::codegen(JIT *jit, bool no_insert) {
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    jit->get_builder().SetInsertPoint(bb);
    args_alloc = CodegenUtils::load_wrapper_input_args(jit, mfunction);
    codegen_done = true;
}

/*
 * ExternArgLoaderIB
 */

void ExternArgLoaderIB::codegen(JIT *jit, bool no_insert) {
    assert(wrapper_input_arg_alloc);
    assert(loop_idx_alloc);
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    jit->get_builder().SetInsertPoint(bb);
    extern_input_arg_alloc = CodegenUtils::load_extern_input_arg(jit, mfunction, wrapper_input_arg_alloc,
                                                                 loop_idx_alloc);
    codegen_done = true;
}

/*
 * LoopCountersIB
 */

llvm::AllocaInst *LoopCountersIB::get_loop_idx_alloc() {
    return loop_idx_alloc;
}

llvm::AllocaInst *LoopCountersIB::get_max_loop_bound_alloc() {
    return max_loop_bound_alloc;
}

llvm::AllocaInst *LoopCountersIB::get_output_idx_alloc() {
    return output_idx_alloc;
}

llvm::AllocaInst *LoopCountersIB::get_malloc_size_alloc() {
    return malloc_size_alloc;
}

void LoopCountersIB::set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound) {
    this->max_loop_bound_alloc = max_loop_bound;
}

void LoopCountersIB::codegen(JIT *jit, bool no_insert) {
    assert(max_loop_bound_alloc);
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    jit->get_builder().SetInsertPoint(bb);
    loop_idx_alloc = CodegenUtils::init_i64(jit, 0, "loop_idx_alloc");
    output_idx_alloc = CodegenUtils::init_i64(jit, 0, "output_idx_alloc");
    malloc_size_alloc = CodegenUtils::init_i64(jit, 0, "malloc_size_alloc");
    codegen_done = true;
}

/*
 * WrapperOutputStructIB
 */

llvm::AllocaInst *WrapperOutputStructIB::get_wrapper_output_struct_alloc() {
    return wrapper_output_struct_alloc;
}

void WrapperOutputStructIB::set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound_alloc) {
    this->max_loop_bound_alloc = max_loop_bound_alloc;
}

void WrapperOutputStructIB::set_malloc_size_alloc(llvm::AllocaInst *malloc_size_alloc) {
    this->malloc_size_alloc = malloc_size_alloc;
}

void WrapperOutputStructIB::codegen(JIT *jit, bool no_insert) {
    assert(max_loop_bound_alloc);
    assert(malloc_size_alloc);
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    jit->get_builder().SetInsertPoint(bb);
    wrapper_output_struct_alloc = CodegenUtils::init_wrapper_output_struct(jit, mfunction, max_loop_bound_alloc,
                                                                           malloc_size_alloc);
    codegen_done = true;
}

/*
 * ForLoopConditionIB
 */

llvm::Value *ForLoopConditionIB::get_loop_comparison() {
    return comparison;
}

void ForLoopConditionIB::set_loop_idx_alloc(llvm::AllocaInst *loop_idx) {
    this->loop_idx_alloc = loop_idx;
}

void ForLoopConditionIB::set_max_loop_bound_alloc(llvm::AllocaInst *max_loop_bound) {
    this->max_loop_bound_alloc = max_loop_bound;
}

// this should be the last thing called after all the optimizations and such are performed
void ForLoopConditionIB::codegen(JIT *jit, bool no_insert) {
    assert(loop_idx_alloc);
    assert(max_loop_bound_alloc);
    assert(mfunction);
    assert(!codegen_done);
    if (bb->getParent() == nullptr) {
        bb->insertInto(mfunction->get_extern_wrapper());
    }
    jit->get_builder().SetInsertPoint(bb);
    comparison = CodegenUtils::create_loop_condition_check(jit, loop_idx_alloc, max_loop_bound_alloc);
    codegen_done = true;
}

/*
 * ForLoopIncrementIB
 */

void ForLoopIncrementIB::set_loop_idx_alloc(llvm::AllocaInst *loop_idx) {
    this->loop_idx_alloc = loop_idx;
}

void ForLoopIncrementIB::codegen(JIT *jit, bool no_insert) {
    assert(loop_idx_alloc);
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    jit->get_builder().SetInsertPoint(bb);
    CodegenUtils::increment_i64(jit, loop_idx_alloc);
    codegen_done = true;
}

/*
 * ForLoopEndIB
 */


void ForLoopEndIB::set_wrapper_output_struct(llvm::AllocaInst *wrapper_output_struct_alloc) {
    this->wrapper_output_struct_alloc = wrapper_output_struct_alloc;
}

void ForLoopEndIB::set_output_idx_alloc(llvm::AllocaInst *output_idx) {
    this->output_idx_alloc = output_idx;
}

void ForLoopEndIB::codegen(JIT *jit, bool no_insert) {
    assert(wrapper_output_struct_alloc);
    assert(output_idx_alloc);
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    jit->get_builder().SetInsertPoint(bb);
    CodegenUtils::return_data(jit, wrapper_output_struct_alloc, output_idx_alloc);
    codegen_done = true;
}

/*
 * ExternCallIB
 */

llvm::AllocaInst *ExternCallIB::get_extern_call_result_alloc() {
    return extern_call_result_alloc;
}

llvm::AllocaInst *ExternCallIB::get_secondary_extern_call_result_alloc() {
    return secondary_extern_call_result_alloc;
}

void ExternCallIB::set_extern_arg_allocs(std::vector<llvm::AllocaInst *> extern_args) {
    this->extern_arg_allocs = extern_args;
}

// used for a stage like FilterStage which needs to store the input data rather than the output of the filter function
void ExternCallIB::set_secondary_extern_call_result_alloc(llvm::AllocaInst *secondary_extern_call_result) {
    this->extern_call_result_alloc = secondary_extern_call_result;
}

void ExternCallIB::codegen(JIT *jit, bool no_insert) {
    assert(!extern_arg_allocs.empty());
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    if (!no_insert) {
        jit->get_builder().SetInsertPoint(bb);
    }
    extern_call_result_alloc = CodegenUtils::create_extern_call(jit, mfunction, extern_arg_allocs);
    codegen_done = true;
}

/*
 * ExternCallStoreIB
 */

void ExternCallStoreIB::set_wrapper_output_struct_alloc(llvm::AllocaInst *return_struct) {
    this->wrapper_output_struct_alloc = return_struct;
}

void ExternCallStoreIB::set_output_idx_alloc(llvm::AllocaInst *return_idx) {
    this->output_idx_alloc = return_idx;
}

void ExternCallStoreIB::set_data_to_store(llvm::AllocaInst *extern_call_result) {
    this->data_to_store_alloc = extern_call_result;
}

void ExternCallStoreIB::set_malloc_size(llvm::AllocaInst *malloc_size) {
    this->malloc_size_alloc = malloc_size;
}

void ExternCallStoreIB::codegen(JIT *jit, bool no_insert) {
    assert(wrapper_output_struct_alloc);
    assert(output_idx_alloc);
    assert(data_to_store_alloc);
    assert(malloc_size_alloc);
    assert(mfunction);
    assert(!codegen_done);
    bb->insertInto(mfunction->get_extern_wrapper());
    jit->get_builder().SetInsertPoint(bb);
    CodegenUtils::store_result(wrapper_output_struct_alloc, jit, output_idx_alloc, data_to_store_alloc, bb->getParent(),
                               malloc_size_alloc, this, mfunction);
    codegen_done = true;
}

/*
 * ExternInitBasicBlock
 */

llvm::AllocaInst *ExternArgLoaderIB::get_extern_input_arg_alloc() {
    return extern_input_arg_alloc;
}

void ExternArgLoaderIB::set_wrapper_input_arg_alloc(llvm::AllocaInst *input_arg_alloc) {
    this->wrapper_input_arg_alloc = input_arg_alloc;
}

void ExternArgLoaderIB::set_loop_idx_alloc(llvm::AllocaInst *loop_idx) {
    this->loop_idx_alloc = loop_idx;
}

