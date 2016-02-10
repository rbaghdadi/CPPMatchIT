//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <openssl/md5.h>
#include "./MType.h"
#include "./Utils.h"
#include "../apps/file_dups/paths.h"
#include "./CompositeTypes.h"

int main(int ac, char **av) {

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

    MArrayType *t = new MArrayType(create_type<char>());
    FileType *f = new FileType();
    ElementType *e = new ElementType(create_type<float>());
    WrapperOutputType *w = new WrapperOutputType(e);
//    t->dump();
//    f->dump();
//    e->dump();
//    w->dump();

    // { i8*, i32, i32 }
    t->codegen()->dump();
    // { { i8*, i32, i32 }* }
    f->codegen()->dump();
    // { { i8*, i32, i32 }*, { float*, i32, i32 }* }
    e->codegen()->dump();
    // { { { { i8*, i32, i32 }*, { float*, i32, i32 }* }**, i32, i32 }* }
    // the ** is the pointer to a bunch of ElementType pointers
    w->codegen()->dump();

    return 0;
}
