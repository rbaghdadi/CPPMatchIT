//
// Created by Jessica Ray on 2/18/16.
//

#include "./StageFactory.h"

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