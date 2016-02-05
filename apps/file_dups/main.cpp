//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <openssl/md5.h>
#include "../../src/LLVM.h"
#include "../../src/Pipeline.h"
#include "./Stages.h"
#include "./paths.h"

// Basic structure of a program:
// 1. User extends necessary block classes
// 2. User creates the necessary "apply" functions for each extended block
// 3. User adds blocks to the pipeline
// 4. User points to the actual input data
// 5. User JITs the code

int main(int ac, char **av) {

    /*
     * Init LLVM and setup the JIT
     */

    LLVM::init();
    JIT jit;

    /*
     * Get some data
     */

    std::vector<const void *> files;
    for (int i = 0; i < 148; i++) {
        std::string s = paths[i];
        char dest[s.size() + 1];
        strncpy(dest, s.c_str(), s.size());
        dest[s.size()] = '\0';
        File *mfile = new File(s.size() + 1);
        mfile->add(dest, s.size() + 1);
        files.push_back(mfile);
    }

    /*
     * Create some stages
     */

    IdentityTransform ixform(identity, &jit);
    Transform xform(transform, &jit);
    Filter filt(llvm_filter, "llvm_filter", &jit);
    HashCompare comp(compare, &jit);
    CompFilter discard(match_filter, "match_filter", &jit);

    /*
     * Run
     */

    Pipeline pipeline;
    pipeline.register_stage(&filt);
    pipeline.register_stage(&xform);
//    pipeline.register_stage(&ixform);
    pipeline.register_stage(&comp);
//    pipeline.register_stage(&discard);
    pipeline.codegen(&jit, files.size());
    jit.dump();
    jit.add_module();
    pipeline.simple_execute(&jit, &(files[0]));

    return 0;
}
