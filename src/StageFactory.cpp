//
// Created by Jessica Ray on 2/18/16.
//

#include "./StageFactory.h"

// TODO do I even need input relations?

TransformStage create_transform_stage(JIT *jit, void (*transform)(const SetElement * const, SetElement * const),
                                      std::string transform_name, Relation *input_relation,
                                      Relation *output_relation) {
    TransformStage stage(transform, transform_name, jit, input_relation, output_relation);
    stage.init_stage();
    return stage;
}

FilterStage create_filter_stage(JIT *jit, bool (*filter)(const SetElement * const), std::string filter_name,
                                Relation *input_relation) {
    FilterStage stage(filter, filter_name, jit, input_relation);
    stage.init_stage();
    return stage;
}


ComparisonStage create_comparison_stage(JIT *jit, bool (*compareBIO)(const SetElement * const, const SetElement * const,
                                                                     SetElement * const), std::string comparison_name,
                                        Relation *input_relation, Relation *output_relation) {
    ComparisonStage stage(compareBIO, comparison_name, jit, input_relation, output_relation);
    stage.init_stage();
    return stage;
}

ComparisonStage create_comparison_stage(JIT *jit, bool (*compareBI)(const SetElement * const, const SetElement * const),
                                        std::string comparison_name, Relation *input_relation) {
    ComparisonStage stage(compareBI, comparison_name, jit, input_relation);
    stage.init_stage();
    return stage;
}

SegmentationStage create_segmentation_stage(JIT *jit,
                                            unsigned int (*segment)(const SetElement *const, SetElement **const),
                                            std::string segmentation_name, Relation *input_relation,
                                            Relation *output_relation, BaseField *field_to_segment,
                                            unsigned int segment_size, float overlap) {
    SegmentationStage stage(segment, segmentation_name, jit, input_relation, output_relation, segment_size, overlap,
                      field_to_segment);
    stage.init_stage();
    return stage;
}
