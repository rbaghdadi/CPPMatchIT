//
// Created by Jessica Ray on 4/20/16.
//

#include "../../src/Init.h"
#include "../../src/JIT.h"
#include "./MF.h"

using namespace MF;

int main() {

    JIT *jit = init();
    setup(jit);
    start(jit);
    cleanup(jit);

    return 0;
}
