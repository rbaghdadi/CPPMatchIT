//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

class SegmentationStage : public Stage {
private:

    unsigned int (*segment)(const SetElement * const, SetElement ** const);
    unsigned int segment_size;
    float overlap;
    BaseField *field_to_segment;

public:

    // field_to_segment: the field that will actually be segmented
    SegmentationStage(unsigned int (*segment)(const SetElement * const, SetElement ** const), std::string segmentation_name,
                      JIT *jit, Relation *input_relation, Relation *output_relation,
                      unsigned int segment_size, float overlap, BaseField *field_to_segment) :
            Stage(jit, "SegmentationStage", segmentation_name, input_relation, output_relation,
                  MScalarType::get_int_type()), segment(segment), segment_size(segment_size), overlap(overlap),
            field_to_segment(field_to_segment) { }

    bool is_segmentation();

//    llvm::Value *compute_preallocation_data_array_size() {
//        return jit->get_builder().CreateMul(compute_num_segments(), CodegenUtils::get_i64(segment_size));
//    }

    llvm::Value *compute_num_segments();

    llvm::Value *compute_num_segments(BaseField *output_field, llvm::Value *loop_bound);

    void handle_extern_output(llvm::AllocaInst *output_data_array_size) { }

//    void finalize_data_array_size(llvm::AllocaInst *output_data_array_size) {
//        llvm::Value *x = jit->get_builder().CreateMul(compute_num_segments(), CodegenUtils::get_i64(segment_size));
//        jit->get_builder().CreateStore(x, output_data_array_size);
//    }

    llvm::AllocaInst *compute_num_output_structs();

    void handle_extern_output(std::vector<llvm::AllocaInst *> preallocated_space);

    llvm::Value *compute_segments_per_setelement();

    BaseField *get_field_to_segment() {
        return field_to_segment;
    }

};


#endif //MATCHIT_SEGMENTATIONSTAGE_H
