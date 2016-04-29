//
// Created by Jessica Ray on 2/18/16.
//

#include "./StageFactory.h"

// TODO do I even need input relations?

TransformStage create_transform_stage(JIT *jit, void (*transform)(const Element * const, Element * const),
                                      std::string transform_name, Fields *input_relation,
                                      Fields *output_relation) {
    TransformStage stage(transform, transform_name, jit, input_relation, output_relation);
    stage.init_stage();
    return stage;
}

FilterStage create_filter_stage(JIT *jit, bool (*filter)(const Element * const), std::string filter_name,
                                Fields *input_relation) {
    FilterStage stage(filter, filter_name, jit, input_relation);
    stage.init_stage();
    return stage;
}


ComparisonStage create_comparison_stage(JIT *jit, bool (*compareBIO)(const Element * const, const Element * const,
                                                                     Element * const), std::string comparison_name,
                                        Fields *input_relation, Fields *output_relation) {
    ComparisonStage stage(compareBIO, comparison_name, jit, input_relation, output_relation);
    stage.init_stage();
    return stage;
}

ComparisonStage create_comparison_stage(JIT *jit, bool (*compareBI)(const Element * const, const Element * const),
                                        std::string comparison_name, Fields *input_relation) {
    ComparisonStage stage(compareBI, comparison_name, jit, input_relation);
    stage.init_stage();
    return stage;
}

SegmentationStage create_segmentation_stage(JIT *jit, unsigned int (*segment)(const Element *const, Element **const),
                                            unsigned int (*compute_num_segments)(const Element *const),
                                            std::string segmentation_name, std::string compute_num_segments_name,
                                            Fields *input_relation, Fields *output_relation,
                                            BaseField *field_to_segment, unsigned int segment_size, float overlap) {
    SegmentationStage stage(segment, compute_num_segments, segmentation_name,
                            compute_num_segments_name, jit,
                            input_relation, output_relation,
                            segment_size, overlap, field_to_segment);
    stage.init_stage();
    return stage;
}
