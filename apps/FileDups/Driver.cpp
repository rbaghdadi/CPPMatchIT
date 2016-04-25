//
// Created by Jessica Ray on 4/4/16.
//


#include "../../src/JIT.h"
#include "../../src/Init.h"
#include "./FileDups.h"

using namespace FileDups;

int main() {

    JIT *jit = init();
    setup(jit);
    for (int i = 0; i < 10; i++) {
        start(jit);
    }
    cleanup(jit);

    return 0;
}