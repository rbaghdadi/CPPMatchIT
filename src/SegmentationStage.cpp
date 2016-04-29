//
// Created by Jessica Ray on 4/1/16.
//

#include "./SegmentationStage.h"

bool SegmentationStage::is_segmentation() {
    return true;
}

//// TODO once switch to variable lengths, will have to check that this doesn't return a negative as the actual data size could be <= seg_size*overlap
//llvm::Value *SegmentationStage::compute_num_segments_old(llvm::Value *loop_bound, llvm::Function *insert_into) {
//    llvm::Value *segment_size_float =
//            llvm::ConstantFP::get(llvm_float, (float)segment_size);
//    llvm::Value *overlap_float =
//            llvm::ConstantFP::get(llvm_float, overlap);
//    llvm::Value *numerator =
//            jit->get_builder().CreateFSub(jit->get_builder().CreateUIToFP(as_i32(field_to_segment->get_fixed_size()),
//                                                                          llvm_float), jit->get_builder().CreateFMul(segment_size_float,
//                                                                                          overlap_float));
//    llvm::Value *denominator =
//            jit->get_builder().CreateFSub(segment_size_float, jit->get_builder().CreateFMul(segment_size_float,
//                                                                                            overlap_float));
//    llvm::Value *num_segments = codegen_llvm_mul(jit,
//            jit->get_builder().CreateFPToSI(codegen_llvm_ceil(jit, jit->get_builder().CreateFDiv(numerator, denominator)),
//                                            llvm_int32), loop_bound);
//    return num_segments;
//}

llvm::AllocaInst *SegmentationStage::compute_num_output_elements() {
    return stage_arg_loader->get_args_alloc()[3];
}

void SegmentationStage::handle_user_function_output() {
    llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_user_function_call_result_alloc(), 4);
    loop->codegen_return_idx_increment(call_result_load);
}