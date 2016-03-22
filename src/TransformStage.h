//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMSTAGE_H
#define MATCHIT_TRANSFORMSTAGE_H

#include "./Stage.h"
#include "./MFunc.h"
#include "./MType.h"
#include "./CodegenUtils.h"

class TransformStage : public Stage {

private:

    void (*transform)(const SetElement * const , SetElement * const);
//    unsigned int transform_size;
//    bool is_fixed_transform_size = true;

public:

//    TransformStage(void (*transform)(const I*, O*), std::string transform_name, JIT *jit, MType *param_type,
//                   MType *return_type, bool is_fixed_size = false, unsigned int fixed_size = 0) :
//            Stage(jit, "TransformStage", transform_name, param_type, return_type, MScalarType::get_void_type(),:
//            is_fixed_size, fixed_size),
//            transform(transform) {}

//    TransformStage(void (*transform)(const SetElement * const, SetElement * const), std::string transform_name,
//                   JIT *jit, std::vector<MType *> param_types, std::vector<BaseField *> fields) :
//            Stage(jit, "TransformStage", transform_name, param_types, MScalarType::get_void_type(), fields),
//            transform(transform) {}

    TransformStage(void (*transform)(const SetElement * const, SetElement * const), std::string transform_name,
                   JIT *jit, Relation *input_relation, Relation *output_relation) :
            Stage(jit, "TransformStage", transform_name, input_relation, output_relation, MScalarType::get_void_type()),
            transform(transform) { }

    ~TransformStage() { }

    bool is_transform() {
        return true;
    }
};

#endif //MATCHIT_TRANSFORMSTAGE_H
