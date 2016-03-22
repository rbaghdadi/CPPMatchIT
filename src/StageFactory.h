//
// Created by Jessica Ray on 2/18/16.
//

#ifndef MATCHIT_STAGEFACTORY_H
#define MATCHIT_STAGEFACTORY_H

#include "./TransformStage.h"
#include "./Field.h"

TransformStage create_transform_stage(JIT *jit, void(*transform)(const SetElement * const, SetElement * const),
                                                  std::string transform_name, Relation *input_relation, Relation *output_relation) {
    std::vector<MType *> input_relation_types = input_relation->get_mtypes();
    std::vector<MType *> output_relation_types = output_relation->get_mtypes();

//    MType *param_type = create_type<SetElement>();
//    MType *return_type = create_type<SetElement>();
    TransformStage stage(transform, transform_name, jit, input_relation, output_relation);//output_relation_types, output_relation->get_fields());
    stage.init_stage();
    return stage;
};


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
