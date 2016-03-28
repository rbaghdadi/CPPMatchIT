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

ComparisonStage create_comparison_stage(JIT *jit, void (*compareVIO)(const SetElement * const, const SetElement * const,
                                                                     SetElement * const), std::string comparison_name,
                                        Relation *input_relation, Relation *output_relation) {
    ComparisonStage stage(compareVIO, comparison_name, jit, input_relation, output_relation);
    stage.init_stage();
    return stage;
}
