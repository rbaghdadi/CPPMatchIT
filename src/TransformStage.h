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

public:

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
