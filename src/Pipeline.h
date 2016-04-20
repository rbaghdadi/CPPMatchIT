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

    /**
     * The individual stages that make up this pipeline.
     */
    std::vector<Stage *> stages;

public:

    ~Pipeline() {}

    /**
     * Add a new stage to the pipeline.
     */
    void register_stage(Stage *stage);

    /**
     * Generate LLVM code for the pipeline
     */
    void codegen(JIT *jit);

};

#endif //MATCHIT_PIPELINE_H
