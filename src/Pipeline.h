//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_PIPELINE_H
#define MATCHIT_PIPELINE_H

#include <vector>
#include "llvm/IR/Type.h"
#include "./JIT.h"
#include "./Stage.h"
#include "./Structures.h"
#include "./Input.h"

class Pipeline {

private:

    std::vector<Stage *> stages;

public:

    ~Pipeline() {}

    void register_stage(Stage *stage);

    // a simple all at once execution for right now
    void simple_execute(JIT *jit, const void **data);

    template <typename T>
    void simple_execute(JIT *jit, Input<T> *input) {
        simple_execute(jit, (const void**)(&(input->get_elements()[0])));
    }

    void execute(JIT *jit, const void **data, long total_bytes_in_arrays, long total_elements);

    // the main entry point to running the pipeline
    void codegen(JIT *jit, size_t num_prim_values, size_t num_structs);

};


#endif //MATCHIT_PIPELINE_H
