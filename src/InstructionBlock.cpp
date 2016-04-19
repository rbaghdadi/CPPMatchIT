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

llvm::AllocaInst *StageArgLoader::get_num_data_structs() {
    return args_alloc[1]; // 2nd element is the number of input SetElements
}

void StageArgLoader::add_arg_alloc(llvm::AllocaInst *alloc) {
    args_alloc.push_back(alloc);
}

void StageArgLoader::codegen(JIT *jit, bool no_insert) {
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    args_alloc = Codegen::load_wrapper_input_args(jit, function);
    codegen_done = true;
}

/*
 * UserFunctionArgLoader
 */

std::vector<llvm::AllocaInst *> UserFunctionArgLoader::get_user_function_input_allocs() {
    return extern_input_arg_alloc;
}

llvm::AllocaInst *UserFunctionArgLoader::get_preallocated_output_space() {
    return preallocated_output_space;
}

void UserFunctionArgLoader::set_stage_input_arg_alloc(std::vector<llvm::AllocaInst *> wrapper_input_arg_alloc) {
    this->stage_input_arg_alloc = wrapper_input_arg_alloc;
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
    extern_input_arg_alloc = Codegen::load_user_function_input_arg(jit, stage_input_arg_alloc,
                                                                   preallocated_output_space, loop_idx_alloc,
                                                                   is_segmentation_stage,
                                                                   is_filter_stage, has_output_param, output_idx_alloc);
    codegen_done = true;
}

/*
 * UserFunctionCall
 */

llvm::AllocaInst *UserFunctionCall::get_extern_call_result_alloc() {
    return extern_call_result_alloc;
}

void UserFunctionCall::set_extern_arg_allocs(std::vector<llvm::AllocaInst *> extern_args) {
    this->extern_arg_allocs = extern_args;
}

void UserFunctionCall::set_extern_function(llvm::Function *extern_function) {
    this->extern_function = extern_function;
}

void UserFunctionCall::codegen(JIT *jit, bool no_insert) {
    assert(!extern_arg_allocs.empty());
    assert(extern_function);
    assert(!codegen_done);
    if (!no_insert) {
        jit->get_builder().SetInsertPoint(bb);
    }
    extern_call_result_alloc = Codegen::create_extern_call(jit, extern_function, extern_arg_allocs);
    codegen_done = true;
}

/*
 * Preallocator
 */

void Preallocator::set_num_elements_to_alloc(llvm::AllocaInst *num_elements_to_alloc) {
    this->num_elements_to_alloc = num_elements_to_alloc;
}

void Preallocator::set_fixed_size(int fixed_size) {
    this->fixed_size = fixed_size;
}

void Preallocator::set_data_array_size(llvm::Value *data_array_size) {
    this->data_array_size = data_array_size;
}

void Preallocator::set_base_type(BaseField *base_type) {
    this->base_field = base_type;
}

void Preallocator::set_input_data(llvm::AllocaInst *input_data) {
    this->input_data = input_data;
}

void Preallocator::set_preallocate_outer_only(bool preallocate_outer_only) {
    this->preallocate_outer_only = preallocate_outer_only;
}

// no, this keeps getting overwritten
llvm::AllocaInst *Preallocator::get_preallocated_space() {
    return preallocated_space;
}

void Preallocator::codegen(JIT *jit, bool no_insert) {
    assert(num_elements_to_alloc);
    assert(base_field);
    assert(fixed_size != 0);
    llvm::Value *size_per_element =
            codegen_llvm_mul(jit, as_i32(base_field->get_fixed_size()),
                                      as_i32(base_field->get_data_mtype()->underlying_size()));
    llvm::Value *total_space_to_preallocate = Codegen::codegen_llvm_mul(jit, size_per_element, codegen_llvm_load(jit, num_elements_to_alloc, 4));
    preallocated_space = preallocate_field(jit, base_field, total_space_to_preallocate);
}

