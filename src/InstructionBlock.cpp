//
// Created by Jessica Ray on 1/31/16.
//

#include "./InstructionBlock.h"
#include "./Utils.h"
#include "./CodegenUtils.h"
#include "./Field.h"
#include "./Preallocator.h"

using namespace Codegen;

/*
 * InstructionBlock
 */

llvm::BasicBlock *InstructionBlock::get_basic_block() {
    return bb;
}

void InstructionBlock::insert(llvm::Function *function) {
    assert(bb);
    this->function = function;
    bb->insertInto(function);
}

/*
 * StageArgLoader
 */

std::vector<llvm::AllocaInst *> StageArgLoader::get_args_alloc() {
    return args_alloc;
}

llvm::AllocaInst *StageArgLoader::get_data(int data_idx) {
    return args_alloc[data_idx];
}

llvm::AllocaInst *StageArgLoader::get_data_array_size() {
    assert(false); // no one should be calling this currently
    return args_alloc[args_alloc.size() - 2]; // 2nd to last element
}

llvm::AllocaInst *StageArgLoader::get_num_input_elements() {
    return args_alloc[1]; // 2nd element is the number of input Elements
}

void StageArgLoader::add_arg_alloc(llvm::AllocaInst *alloc) {
    args_alloc.push_back(alloc);
}

void StageArgLoader::codegen(JIT *jit, bool no_insert) {
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    args_alloc = Codegen::load_stage_input_params(jit, function);
    codegen_done = true;
}

/*
 * UserFunctionArgLoader
 */

std::vector<llvm::AllocaInst *> UserFunctionArgLoader::get_user_function_input_allocs() {
    return user_function_input_arg_alloc;
}

llvm::AllocaInst *UserFunctionArgLoader::get_preallocated_output_space() {
    return preallocated_output_space;
}

void UserFunctionArgLoader::set_stage_input_arg_alloc(std::vector<llvm::AllocaInst *> stage_input_arg_alloc) {
    this->stage_input_arg_alloc = stage_input_arg_alloc;
}

void UserFunctionArgLoader::set_no_output_param() {
    has_output_param = false;
}

void UserFunctionArgLoader::set_loop_idx_alloc(std::vector<llvm::AllocaInst *> loop_idx) {
    this->loop_idx_alloc = loop_idx;
}

void UserFunctionArgLoader::set_segmentation_stage() {
    is_segmentation_stage = true;
}

void UserFunctionArgLoader::set_filter_stage() {
    is_filter_stage = true;
}

void UserFunctionArgLoader::set_preallocated_output_space(llvm::AllocaInst *preallocated_output_space) {
    this->preallocated_output_space = preallocated_output_space;
}

void UserFunctionArgLoader::set_output_idx_alloc(llvm::AllocaInst *output_idx_alloc) {
    this->output_idx_alloc = output_idx_alloc;
}

void UserFunctionArgLoader::codegen(JIT *jit, bool no_insert) {
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    user_function_input_arg_alloc = Codegen::load_user_function_input_param(jit, stage_input_arg_alloc,
                                                                            preallocated_output_space, loop_idx_alloc,
                                                                            is_segmentation_stage,
                                                                            is_filter_stage, has_output_param,
                                                                            output_idx_alloc);
    codegen_done = true;
}

/*
 * UserFunctionCall
 */

llvm::AllocaInst *UserFunctionCall::get_user_function_call_result_alloc() {
    return user_function_call_result_alloc;
}

void UserFunctionCall::set_user_function_arg_allocs(std::vector<llvm::AllocaInst *> user_function_args) {
    this->user_function_arg_allocs = user_function_args;
}

void UserFunctionCall::set_user_function(llvm::Function *user_function) {
    this->user_function = user_function;
}

void UserFunctionCall::codegen(JIT *jit, bool no_insert) {
    assert(!user_function_arg_allocs.empty());
    assert(user_function);
    assert(!codegen_done);
    if (!no_insert) {
        jit->get_builder().SetInsertPoint(bb);
    }
    user_function_call_result_alloc = Codegen::create_user_function_call(jit, user_function, user_function_arg_allocs);
    codegen_done = true;
}

/*
 * Preallocator
 */

void Preallocator::set_num_elements_to_alloc(llvm::AllocaInst *num_elements_to_alloc) {
    this->num_elements_to_alloc = num_elements_to_alloc;
}

void Preallocator::set_base_field(BaseField *base_field) {
    this->base_field = base_field;
}

// no, this keeps getting overwritten
llvm::AllocaInst *Preallocator::get_preallocated_space() {
    return preallocated_space;
}

void Preallocator::codegen(JIT *jit, bool no_insert) {
    assert(num_elements_to_alloc);
    assert(base_field);
    llvm::Value *size_per_element =
            codegen_llvm_mul(jit, as_i32(base_field->get_fixed_size()),
                                      as_i32(base_field->get_data_mtype()->underlying_size()));
    llvm::Value *total_space_to_preallocate = Codegen::codegen_llvm_mul(jit, size_per_element, codegen_llvm_load(jit, num_elements_to_alloc, 4));
    preallocated_space = preallocate_field(jit, base_field, total_space_to_preallocate);
}

