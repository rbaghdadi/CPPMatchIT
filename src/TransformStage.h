//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMSTAGE_H
#define MATCHIT_TRANSFORMSTAGE_H

#include <string>
#include "./Stage.h"
#include "./MType.h"

class TransformStage : public Stage {
private:

    void (*transform)(const Element * const, Element * const);

public:

    TransformStage(void (*transform)(const Element * const, Element * const), std::string transform_name,
                   JIT *jit, Fields *input_relation, Fields *output_relation) :
            Stage(jit, "TransformStage", transform_name, input_relation, output_relation, MScalarType::get_void_type()),
            transform(transform) { }

    ~TransformStage() { }

    bool is_transform();

};

#endif //MATCHIT_TRANSFORMSTAGE_H
