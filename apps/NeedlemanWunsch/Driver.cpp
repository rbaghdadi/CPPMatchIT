//
// Created by Jessica Ray on 4/6/16.
//

#include <fstream>
#include "../../src/Init.h"
#include "./NW.h"

using namespace NW;

int main() {

    JIT *jit = init();
    setup(jit);
    for (int i = 0; i < 10; i++) {
        start(jit);
    }
    cleanup(jit);

    return 0;
}
