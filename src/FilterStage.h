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

    bool (*filter)(const Element * const);

public:

    FilterStage(bool (*filter)(const Element * const), std::string filter_name, JIT *jit, Fields *input_relation) :
            Stage(jit, "FilterStage", filter_name, input_relation, new Fields(), MScalarType::get_bool_type()),
            filter(filter) { }

    ~FilterStage() {}

    bool is_filter();

    void handle_extern_output();

    std::vector<llvm::AllocaInst *> preallocate();

};

#endif //MATCHIT_FILTERBLOCK_H
