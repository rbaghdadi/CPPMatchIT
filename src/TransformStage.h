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

    // Only here to enforce that the user's function signature is correct. And for debugging.
    void (*transform)(const Element * const, Element * const) = nullptr;

public:

    TransformStage(void (*transform)(const Element * const, Element * const), std::string transform_name,
                   JIT *jit, Fields *input_relation, Fields *output_relation) :
            Stage(jit, "TransformStage", transform_name, input_relation, output_relation, MScalarType::get_void_type()),
            transform(transform) {
        // trick compiler into thinking this is used
        (void)(this->transform);
    }

    ~TransformStage() { }

    bool is_transform();

};

#endif //MATCHIT_TRANSFORMSTAGE_H
