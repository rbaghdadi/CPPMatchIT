//
// Created by Jessica Ray on 2/18/16.
//

#ifndef MATCHIT_STAGEFACTORY_H
#define MATCHIT_STAGEFACTORY_H

#include "./ComparisonStage.h"
#include "./Field.h"
#include "./FilterStage.h"
#include "./SegmentationStage.h"
#include "./TransformStage.h"

TransformStage create_transform_stage(JIT *jit, void (*transform)(const SetElement * const, SetElement * const),
                                      std::string transform_name, Relation *input_relation,
                                      Relation *output_relation);

FilterStage create_filter_stage(JIT *jit, bool (*filter)(const SetElement * const), std::string filter_name,
                                Relation *input_relation);

ComparisonStage create_comparison_stage(JIT *jit, bool (*compareBIO)(const SetElement * const, const SetElement * const,
                                                                     SetElement * const), std::string comparison_name,
                                        Relation *input_relation, Relation *output_relation);

ComparisonStage create_comparison_stage(JIT *jit, bool (*compareBI)(const SetElement * const, const SetElement * const),
                                        std::string comparison_name, Relation *input_relation);

SegmentationStage create_segmentation_stage(JIT *jit,
                                            unsigned int (*segment)(const SetElement *const, SetElement **const),
                                            std::string segmentation_name, Relation *input_relation,
                                            Relation *output_relation, BaseField *field_to_segment,
                                            unsigned int segment_size, float overlap);

#endif //MATCHIT_STAGEFACTORY_H
