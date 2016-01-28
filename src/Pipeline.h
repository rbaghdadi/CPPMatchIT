//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_PIPELINE_H
#define MATCHIT_PIPELINE_H

#include <vector>
#include "llvm/IR/Type.h"
#include "./JIT.h"
#include "./Block.h"

class Pipeline {

private:

    std::vector<Block *> building_blocks;

public:

    ~Pipeline() {}

    void register_block(Block *block);

    // a simple all at once execution for right now
    void simple_execute(JIT &jit, const void **data);

    // the main entry point to running the pipeline
    void codegen(JIT &jit);

};


#endif //MATCHIT_PIPELINE_H
