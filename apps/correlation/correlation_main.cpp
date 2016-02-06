//
// Created by Jessica Ray on 2/5/16.
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

    std::string file1 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/referent.wav.sw";
    char dest1[file1.size() + 1];
    strncpy(dest1, file1.c_str(), file1.size());
    dest1[file1.size()] = '\0';
    File *ref = new File(file1.size() + 1);
    ref->add(dest1, file1.size() + 1);
    files.push_back(ref);

    std::string file2 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/test2x2.wav.sw";
    char dest2[file2.size() + 1];
    strncpy(dest2, file2.c_str(), file2.size());
    dest2[file2.size()] = '\0';
    File *sig = new File(file2.size() + 1);
    sig->add(dest2, file2.size() + 1);
    files.push_back(sig);

    /*
     * Create some stages
     */

    std::cerr << "I'm running the matched filter!" << std::endl;

    SlidingWindow window(segment, &jit);
    FFTxform fftxform(fft, &jit);
    iFFTxform ifftxform(ifft, &jit);

    /*
     * Run
     */

    Pipeline pipeline;
    pipeline.register_stage(&window);
    pipeline.register_stage(&fftxform);
//    pipeline.register_stage(&ixform);
    pipeline.codegen(&jit, files.size());
    jit.dump();
    jit.add_module();
    pipeline.simple_execute(&jit, &(files[0]));

    return 0;
}
