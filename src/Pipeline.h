//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_PIPELINE_H
#define MATCHIT_PIPELINE_H

#include <vector>
#include "llvm/IR/Type.h"
#include "./JIT.h"
#include "./Stage.h"
#include "Structures.h"

class Pipeline {

private:

    std::vector<Stage *> stages;

public:

    ~Pipeline() {}

    void register_stage(Stage *stage);

    // a simple all at once execution for right now
    void simple_execute(JIT *jit, const void **data);

    void execute(JIT *jit, const void **data, long total_bytes_in_arrays, long total_elements);

    // the main entry point to running the pipeline
    void codegen(JIT *jit, size_t fixed_array_length, size_t total_elements);

};


#endif //MATCHIT_PIPELINE_H
