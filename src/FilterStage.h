//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_FILTERBLOCK_H
#define MATCHIT_FILTERBLOCK_H

#include <string>
#include "./Stage.h"

using namespace Codegen;

class FilterStage : public Stage {

private:

    bool (*filter)(const SetElement * const);

public:

    FilterStage(bool (*filter)(const SetElement * const), std::string filter_name, JIT *jit, Relation *input_relation) :
            Stage(jit, "FilterStage", filter_name, input_relation, new Relation(), MScalarType::get_bool_type()),
            filter(filter) { }

    ~FilterStage() {}

    bool is_filter();

    void handle_extern_output(std::vector<llvm::AllocaInst *> preallocated_space);

    std::vector<llvm::AllocaInst *> preallocate();

};

#endif //MATCHIT_FILTERBLOCK_H
