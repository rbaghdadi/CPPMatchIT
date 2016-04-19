//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_PIPELINE_H
#define MATCHIT_PIPELINE_H

#include <vector>
#include "llvm/IR/Type.h"
#include "./JIT.h"
#include "./Stage.h"
#include "./Field.h"

class Pipeline {

private:

    std::vector<Stage *> stages;

public:

    ~Pipeline() {}

    void register_stage(Stage *stage);

    // a simple all at once execution for right now
//    void simple_execute(JIT *jit, const void **data);

    void codegen(JIT *jit);

};


#endif //MATCHIT_PIPELINE_H
