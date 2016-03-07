//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

template <typename I, typename O>
class SegmentationStage : public Stage {

private:

    void (*segment)(const I*, O**);
    unsigned int segment_size;
    float overlap;

public:

    SegmentationStage(void (*segment)(const I*, O**), std::string segmentation_name, JIT *jit, MType *param_type,
                      MType *return_type, unsigned int segment_size, float overlap) :
            Stage(jit, "SegmentationStage", segmentation_name, param_type, return_type, MScalarType::get_void_type(),
                  true, segment_size),
            segment(segment), segment_size(segment_size), overlap(overlap) { }

    bool is_segmentation() {
        return true;
    }

    llvm::Value *compute_num_segments() {
        llvm::Value *segment_size_float =
                llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), (float)segment_size);
        llvm::Value *overlap_float =
                llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), overlap);
        llvm::Value *num_prim_values_float =
                jit->get_builder().CreateUIToFP(jit->get_builder().CreateLoad(stage_arg_loader->get_data_array_size()),
                                                llvm::Type::getFloatTy(llvm::getGlobalContext()));
        llvm::Value *numerator =
                jit->get_builder().CreateFSub(num_prim_values_float, jit->get_builder().CreateFMul(segment_size_float,
                                                                                                   overlap_float));
        llvm::Value *denominator =
                jit->get_builder().CreateFSub(segment_size_float, jit->get_builder().CreateFMul(segment_size_float,
                                                                                                overlap_float));
        llvm::Value *num_segments =
                jit->get_builder().CreateFPToUI(CodegenUtils::codegen_llvm_ceil(jit, jit->get_builder().CreateFDiv(numerator, denominator)),
                                                                             llvm::Type::getInt64Ty(llvm::getGlobalContext()));
        return num_segments;
    }

    llvm::Value *compute_preallocation_data_array_size() {
        return jit->get_builder().CreateMul(compute_num_segments(), CodegenUtils::get_i64(segment_size));
    }

    void handle_extern_output(llvm::AllocaInst *output_data_array_size) { }

    void finalize_data_array_size(llvm::AllocaInst *output_data_array_size) {
        llvm::Value *x = jit->get_builder().CreateMul(compute_num_segments(), CodegenUtils::get_i64(segment_size));
        jit->get_builder().CreateStore(x, output_data_array_size);
    }

    llvm::AllocaInst *compute_num_output_structs() {
        llvm::Value *num = compute_num_segments();
        llvm::AllocaInst *num_alloca = jit->get_builder().CreateAlloca(num->getType());
        jit->get_builder().CreateStore(num, num_alloca);
        return num_alloca;
    }

};


#endif //MATCHIT_SEGMENTATIONSTAGE_H
