//
// Created by Jessica Ray on 4/22/16.
//

// run all applications

#include "../src/Init.h"
#include "./FileDups/FileDups.h"
#include "./NeedlemanWunsch/NW.h"
#include "./MatchedFilter/MF.h"

int main() {

    int iters = 5;

    // FileDups
    JIT *jit_fd = init();
    FileDups::setup(jit_fd);
    for (int i = 0; i < iters; i++) {
        FileDups::start(jit_fd);
    }
    cleanup(jit_fd);

    // Genome alignment
    JIT *jit_nw = init();
    NW::setup(jit_nw);
    for (int i = 0; i < iters; i++) {
        NW::start(jit_nw);
    }
    cleanup(jit_nw);

    // Matched filter
    JIT *jit_mf = init();
    MF::setup(jit_mf);
    for (int i = 0; i < iters; i++) {
        MF::start(jit_mf);
    }
    cleanup(jit_mf);

}