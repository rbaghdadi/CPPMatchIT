//
// Created by Jessica Ray on 1/30/16.
//

#ifndef MATCHIT_IMPURESTAGE_H
#define MATCHIT_IMPURESTAGE_H

#include "./MType.h"
#include "./Stage.h"

class ImpureStage : public Stage {

public:

    ImpureStage(std::string stage_name, JIT *jit, MFunc *mfunction, mtype_code_t input_mtype_code,
                mtype_code_t output_mtype_code) : Stage(jit, input_mtype_code, output_mtype_code, stage_name) {
        set_function(mfunction);
    }

    void codegen() {

    }

};


#endif //MATCHIT_IMPURESTAGE_H
