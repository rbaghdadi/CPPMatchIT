//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

class SegmentationStage : public Stage {
private:

    unsigned int (*segment)(const Element * const, Element ** const);
    unsigned int segment_size;
    float overlap;
    BaseField *field_to_segment;

public:

    // field_to_segment: the field that will actually be segmented
    SegmentationStage(unsigned int (*segment)(const Element * const, Element ** const), std::string segmentation_name,
                      JIT *jit, Fields *input_relation, Fields *output_relation,
                      unsigned int segment_size, float overlap, BaseField *field_to_segment) :
            Stage(jit, "SegmentationStage", segmentation_name, input_relation, output_relation,
                  MScalarType::get_int_type()), segment(segment), segment_size(segment_size), overlap(overlap),
            field_to_segment(field_to_segment) { }

    bool is_segmentation();

    /*
     * Compute using the following formula:
     *
     *       (total # data points) - ((segment size) * overlap)
     * ceil( -------------------------------------------------- )
     *       (segment size) - ((segment size) * overlap)
     *
     */
    llvm::Value *compute_num_segments(llvm::Value *loop_bound, llvm::Function *insert_into);

    llvm::AllocaInst *compute_num_output_elements();

    void handle_user_function_output();

};

#endif //MATCHIT_SEGMENTATIONSTAGE_H
