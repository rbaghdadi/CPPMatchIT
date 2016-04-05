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

ComparisonStage create_comparison_stage(JIT *jit, void (*compareVIO)(const SetElement * const, const SetElement * const,
                                                                     SetElement * const), std::string comparison_name,
                                        Relation *input_relation, Relation *output_relation);

SegmentationStage create_segmentation_stage(JIT *jit,
                                            unsigned int (*segment)(const SetElement *const, SetElement **const),
                                            std::string segmentation_name, Relation *input_relation,
                                            Relation *output_relation, BaseField *field_to_segment,
                                            unsigned int segment_size, float overlap);


//template <typename I, typename O>
//TransformStage<const I, O> create_transform_stage(JIT *jit, void(*transform)(const I*, O*),
//                                                  std::string transform_name) {
//    MType *param_type = create_type<I>();
//    MType *return_type = create_type<O>();
//    TransformStage<const I, O> stage(transform, transform_name, jit, param_type, return_type);
//    stage.init_stage();
//    return stage;
//};
//
//template <typename I, typename O>
//TransformStage<const I, O> create_transform_stage(JIT *jit, void(*transform)(const I*, O*),
//                                                  std::string transform_name, unsigned int transform_size) {
//    MType *param_type = create_type<I>();
//    MType *return_type = create_type<O>();
//    TransformStage<const I, O> stage(transform, transform_name, jit, param_type, return_type, true, transform_size);
//    stage.init_stage();
//    return stage;
//};
//
//template <typename I>
//FilterStage<const I> create_filter_stage(JIT *jit, bool (*filter)(const I*), std::string filter_name) {
//    MType *param_type = create_type<I>();
//    FilterStage<const I> stage(filter, filter_name, jit, param_type);
//    stage.init_stage();
//    return stage;
//};
//
//template <typename I, typename O>
//SegmentationStage<const I, O> create_segmentation_stage(JIT *jit, void (*segment)(const I*, O**),
//                                                        std::string segmentation_name, unsigned int segment_size,
//                                                        float overlap) {
//    MType *param_type = create_type<I>();
//    MType *return_type = create_type<O>();
//    SegmentationStage<const I, O> stage(segment, segmentation_name, jit, param_type,
//                                        return_type, segment_size, overlap);
//    stage.init_stage();
//    return stage;
//};


#endif //MATCHIT_STAGEFACTORY_H
