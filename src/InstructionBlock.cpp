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

//MFunc *InstructionBlock::get_mfunction() {
//    return mfunction;
//}
//
//void InstructionBlock::set_mfunction(MFunc *mfunction) {
//    this->mfunction = mfunction;
//}

//void InstructionBlock::force_insert(MFunc *mfunction) {
//    bb->insertInto(mfunction->get_extern_wrapper());
//}

void InstructionBlock::insert(llvm::Function *function) {
    assert(bb);
    this->function = function;
    bb->insertInto(function);
}

/*
 * StageArgLoaderIB
 */

std::vector<llvm::AllocaInst *> StageArgLoaderIB::get_args_alloc() {
    return args_alloc;
}

llvm::AllocaInst *StageArgLoaderIB::get_data() {
    return args_alloc[0];
}

llvm::AllocaInst *StageArgLoaderIB::get_data_array_size() {
    return args_alloc[1];
}

llvm::AllocaInst *StageArgLoaderIB::get_num_data_structs() {
    return args_alloc[2];
}

void StageArgLoaderIB::codegen(JIT *jit, bool no_insert) {
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    args_alloc = CodegenUtils::load_wrapper_input_args(jit, function);
    codegen_done = true;
}

/*
 * ExternArgLoaderIB
 */

std::vector<llvm::AllocaInst *> ExternArgLoaderIB::get_extern_input_arg_alloc() {
    return extern_input_arg_alloc;
}

llvm::AllocaInst *ExternArgLoaderIB::get_preallocated_output_space() {
    return preallocated_output_space;
}

void ExternArgLoaderIB::set_stage_input_arg_alloc(llvm::AllocaInst *wrapper_input_arg_alloc) {
    this->stage_input_arg_alloc = wrapper_input_arg_alloc;
}

void ExternArgLoaderIB::set_preallocated_output_space(llvm::AllocaInst *preallocated_output_space) {
    this->preallocated_output_space = preallocated_output_space;
}

void ExternArgLoaderIB::set_loop_idx_alloc(llvm::AllocaInst *loop_idx) {
    this->loop_idx_alloc = loop_idx;
}

void ExternArgLoaderIB::set_segmentation_stage() {
    is_segmentation_stage = true;
}

void ExternArgLoaderIB::codegen(JIT *jit, bool no_insert) {
    assert(stage_input_arg_alloc);
    assert(loop_idx_alloc);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    extern_input_arg_alloc = CodegenUtils::load_extern_input_arg(jit, stage_input_arg_alloc,
                                                                 preallocated_output_space, loop_idx_alloc,
                                                                 is_segmentation_stage);
    codegen_done = true;
}

/*
 * LoopCountersIB
 */

llvm::AllocaInst *ForLoopCountersIB::get_loop_idx_alloc() {
    return loop_idx_alloc;
}

llvm::AllocaInst *ForLoopCountersIB::get_loop_bound_alloc() {
    return loop_bound_alloc;
}

llvm::AllocaInst *ForLoopCountersIB::get_return_idx_alloc() {
    return return_idx_alloc;
}

void ForLoopCountersIB::set_loop_bound_alloc(llvm::AllocaInst *max_loop_bound) {
    this->loop_bound_alloc = max_loop_bound;
}

void ForLoopCountersIB::codegen(JIT *jit, bool no_insert) {
    assert(loop_bound_alloc);
    assert(!codegen_done);
    jit->get_builder().SetInsertPoint(bb);
    loop_idx_alloc = CodegenUtils::init_i64(jit, 0, "loop_idx_alloc");
    return_idx_alloc = CodegenUtils::init_i64(jit, 0, "output_idx_alloc");
    codegen_done = true;
}

///*
// * WrapperOutputStructIB
// */
//
//llvm::AllocaInst *WrapperOutputStructIB::get_wrapper_output_struct_alloc() {
//    return wrapper_output_struct_alloc;
//}
//
//void WrapperOutputStructIB::set_loop_bound_alloc(llvm::AllocaInst *loop_bound_alloc) {
//    this->loop_bound_alloc = loop_bound_alloc;
//}
//
//void WrapperOutputStructIB::set_malloc_size_alloc(llvm::AllocaInst *malloc_size_alloc) {
//    this->malloc_size_alloc = malloc_size_alloc;
//}
//
//void WrapperOutputStructIB::codegen(JIT *jit, bool no_insert) {
//    assert(loop_bound_alloc);
//    assert(malloc_size_alloc);
////    assert(mfunction);
//    assert(!codegen_done);
////    bb->insertInto(mfunction->get_extern_wrapper());
//    jit->get_builder().SetInsertPoint(bb);
////    wrapper_output_struct_alloc = CodegenUtils::init_wrapper_output_struct(jit, mfunction, loop_bound_alloc,
////                                                                           malloc_size_alloc);
//    codegen_done = true;
//}

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
//    assert(mfunction);
    assert(!codegen_done);
    if (bb->getParent() == nullptr) {
//        bb->insertInto(mfunction->get_extern_wrapper());
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
//    assert(mfunction);
    assert(!codegen_done);
//    bb->insertInto(mfunction->get_extern_wrapper());
    if (!no_insert) {
        jit->get_builder().SetInsertPoint(bb);
    }
    CodegenUtils::increment_i64(jit, loop_idx_alloc);
    codegen_done = true;
}

///*
// * ForLoopEndIB
// */
//
//
//void ForLoopEndIB::set_wrapper_output_struct(llvm::AllocaInst *wrapper_output_struct_alloc) {
//    this->wrapper_output_struct_alloc = wrapper_output_struct_alloc;
//}
//
//void ForLoopEndIB::set_output_idx_alloc(llvm::AllocaInst *output_idx) {
//    this->output_idx_alloc = output_idx;
//}
//
//void ForLoopEndIB::codegen(JIT *jit, bool no_insert) {
//    assert(wrapper_output_struct_alloc);
//    assert(output_idx_alloc);
////    assert(mfunction);
//    assert(!codegen_done);
////    bb->insertInto(mfunction->get_extern_wrapper());
//    jit->get_builder().SetInsertPoint(bb);
//    CodegenUtils::return_data(jit, wrapper_output_struct_alloc, output_idx_alloc);
//    codegen_done = true;
//}

/*
 * ExternCallIB
 */

llvm::AllocaInst *ExternCallIB::get_extern_call_result_alloc() {
    return extern_call_result_alloc;
}

void ExternCallIB::set_extern_arg_allocs(std::vector<llvm::AllocaInst *> extern_args) {
    this->extern_arg_allocs = extern_args;
}

void ExternCallIB::set_extern_function(llvm::Function *extern_function) {
    this->extern_function = extern_function;
}

void ExternCallIB::codegen(JIT *jit, bool no_insert) {
    assert(!extern_arg_allocs.empty());
    assert(extern_function);
    assert(!codegen_done);
    if (!no_insert) {
        jit->get_builder().SetInsertPoint(bb);
    }
    extern_call_result_alloc = CodegenUtils::create_extern_call(jit, extern_function, extern_arg_allocs);
    codegen_done = true;
}

/*
 * PreallocatorIB
 */

void PreallocatorIB::set_loop_bound_alloc(llvm::AllocaInst *loop_bound_alloc) {
    this->loop_bound_alloc = loop_bound_alloc;
}

void PreallocatorIB::set_fixed_size(int fixed_size) {
    this->fixed_size = fixed_size;
}

void PreallocatorIB::set_data_array_size(llvm::Value *data_array_size) {
    this->data_array_size = data_array_size;
}

void PreallocatorIB::set_base_type(MType *base_type) {
    this->base_type = base_type;
}

void PreallocatorIB::set_input_data(llvm::AllocaInst *input_data) {
    this->input_data = input_data;
}

void PreallocatorIB::set_preallocate_outer_only(bool preallocate_outer_only) {
    this->preallocate_outer_only = preallocate_outer_only;
}

llvm::AllocaInst *PreallocatorIB::get_preallocated_space() {
    return preallocated_space;
}

/*
 * FixedPreallocatorIB
 */

void FixedPreallocatorIB::codegen(JIT *jit, bool no_insert) {
    assert(!codegen_done);
    assert(loop_bound_alloc);
    assert(base_type);
    assert(data_array_size);
    assert(fixed_size != 0);
    llvm::LoadInst *loop_bound_load = jit->get_builder().CreateLoad(loop_bound_alloc);
    preallocated_space = base_type->preallocate_fixed_block(jit, loop_bound_load, data_array_size,
                                                            CodegenUtils::get_i64(fixed_size), function);
    codegen_done = true;
}

/*
 * MatchedPreallocatorIB
 */

void MatchedPreallocatorIB::codegen(JIT *jit, bool no_insert) {
    assert(!codegen_done);
    assert(loop_bound_alloc);
    assert(base_type);
    assert(data_array_size);
    assert(input_data);
    llvm::LoadInst *loop_bound_load = jit->get_builder().CreateLoad(loop_bound_alloc);
    preallocated_space = base_type->preallocate_matched_block(jit, loop_bound_load, data_array_size, function,
                                                              input_data, preallocate_outer_only);
    codegen_done = true;
}
