//
// Created by Jessica Ray on 4/1/16.
//

#include "./SegmentationStage.h"

bool SegmentationStage::is_segmentation() {
    return true;
}

llvm::Value *SegmentationStage::compute_num_segments(llvm::Value *loop_bound) {
    llvm::Value *segment_size_float =
            llvm::ConstantFP::get(llvm_float, (float)segment_size);
    llvm::Value *overlap_float =
            llvm::ConstantFP::get(llvm_float, overlap);
    llvm::Value *total_array_size = jit->get_builder().CreateUIToFP(codegen_llvm_mul(jit, as_i32(field_to_segment->get_fixed_size()),
                                                                                     loop_bound), llvm_float);
    llvm::Value *numerator =
            jit->get_builder().CreateFSub(total_array_size, jit->get_builder().CreateFMul(segment_size_float,
                                                                                          overlap_float));
    llvm::Value *denominator =
            jit->get_builder().CreateFSub(segment_size_float, jit->get_builder().CreateFMul(segment_size_float,
                                                                                            overlap_float));
    llvm::Value *num_segments =
            jit->get_builder().CreateFPToUI(codegen_llvm_ceil(jit, jit->get_builder().CreateFDiv(numerator, denominator)),
                                            llvm_int32);
    return num_segments;
}

llvm::AllocaInst *SegmentationStage::compute_num_output_structs() {
    llvm::Value *num = compute_num_segments( codegen_llvm_load(jit, loop->get_loop_bound(), 4));
    llvm::AllocaInst *num_alloca = jit->get_builder().CreateAlloca(num->getType());
    jit->get_builder().CreateStore(num, num_alloca);
    return num_alloca;
}

void SegmentationStage::handle_extern_output(std::vector<llvm::AllocaInst *> preallocated_space) {
    llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_extern_call_result_alloc(), 4);
    loop->codegen_return_idx_increment(call_result_load);
}