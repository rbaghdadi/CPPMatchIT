//
// Created by Jessica Ray on 2/9/16.
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "../src/MType.h"
#include "../src/Utils.h"
#include "../src/Structures.h"
#include "../src/TransformStage.h"
#include "../src/LLVM.h"
#include "../src/Pipeline.h"
#include "../apps/file_dups/paths.h"

class IdentityTransform : public TransformStage<File *, File *> {

public:

    IdentityTransform(File *(*transform)(File *), JIT *jit) :
            TransformStage(transform, "identity", jit, new FileType(), new FileType()) {}

};

extern "C" File *identity(File *file) {
//    char md5_str[33];
//    for (int i = 0; i < 16; i++) { // file.get_data(i)
//        sprintf(&md5_str[i * 2], "%02x", file->get_data()->get_array()[i]);
//    }
//    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
    std::cerr << file->get_filepath()->get_array() << std::endl;
    return file;
}

int main() {

    LLVM::init();
    JIT jit;
    register_utils(&jit);

    std::cerr << "Getting data" << std::endl;
    std::vector<const void *> files;
    for (int i = 0; i < 148; i++) {
        std::string s = paths[i];
        char dest[s.size() + 1];
        strncpy(dest, s.c_str(), s.size());
        dest[s.size()] = '\0';
        MArray<char> *filepath = new MArray<char>(dest, s.size() + 1);
        File *mfile = new File();
        mfile->set_filepath(filepath);
        files.push_back(mfile);
        std::cerr << mfile->get_filepath()->get_array() << std::endl;
    }

    IdentityTransform ixform(&identity, &jit);
    Pipeline pipeline;
    pipeline.register_stage(&ixform);
    pipeline.register_stage(&ixform);
    pipeline.codegen(&jit, files.size());
    jit.dump();
    jit.add_module();
    pipeline.simple_execute(&jit, &(files[0]));

    std::cerr << "Done!" << std::endl;
}