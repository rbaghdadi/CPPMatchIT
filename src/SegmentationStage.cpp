//
// Created by Jessica Ray on 4/1/16.
//

#include "./SegmentationStage.h"

bool SegmentationStage::is_segmentation() {
    return true;
}

llvm::AllocaInst *SegmentationStage::compute_num_output_elements() {
    return stage_arg_loader->get_args_alloc()[3];
}

void SegmentationStage::handle_user_function_output() {
    llvm::LoadInst *call_result_load = codegen_llvm_load(jit, call->get_user_function_call_result_alloc(), 4);
    loop->codegen_return_idx_increment(call_result_load);
}