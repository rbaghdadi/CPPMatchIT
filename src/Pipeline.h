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
#include "./Input.h"

class Pipeline {

private:

    std::vector<Stage *> stages;
    std::vector<std::tuple<Stage *, Relation *, Relation *>> paramaterized_stages;

public:

    ~Pipeline() {}

    void register_stage(Stage *stage);

    void register_stage(Stage *stage, Relation *input_relation);

    void register_stage(Stage *stage, Relation *input_relation, Relation *output_relation);

    // a simple all at once execution for right now
    void simple_execute(JIT *jit, const void **data);

    template <typename T>
    void simple_execute(JIT *jit, Input<T> *input) {
        simple_execute(jit, (const void**)(&(input->get_elements()[0])));
    }

//    void simple_execute(JIT *jit, std::vector<SetElement *> input, std::vector<SetElement *> output, BaseField *f1, BaseField *f2) {
//        jit->run("pipeline", (const void**)(&(input[0])), input.size(), (const void**)(&(output[0])), output.size(), (const void*)f1, (const void*)f2);
//    }

    void execute(JIT *jit, const void **data, long total_bytes_in_arrays, long total_elements);

    void codegen(JIT *jit);

};


#endif //MATCHIT_PIPELINE_H
