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

    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 3
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    MArrayType *t = new MArrayType(create_type<char>());

    //FileType has field type MPointerType pointing to:
    //        MArrayType data type: MPointerType pointing to:
    //        MPrimType with type code: 3
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    FileType *f = new FileType();

    //ElementType has field types MPointerType pointing to:
    //        MArrayType data type: MPointerType pointing to:
    //        MPrimType with type code: 3
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    //and MPointerType pointing to:
    //        MArrayType data type: MPointerType pointing to:
    //        MPrimType with type code: 3
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    ElementType *e = new ElementType(create_type<float>());

    // Element with type float
    //WrapperOutputType has field types MPointerType pointing to:
    //        MArrayType data type: MPointerType pointing to:
    //        MPointerType pointing to:
    //ElementType has field types MPointerType pointing to:
    //        MArrayType data type: MPointerType pointing to:
    //        MPrimType with type code: 3
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    //and MPointerType pointing to:
    //        MArrayType data type: MPointerType pointing to:
    //        MPrimType with type code: 7
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    WrapperOutputType *w = new WrapperOutputType(f);

    //SegmentedElementType has field types MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 3
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    //                          and MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 7
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    //                          and MPrimType with type code: 5
    SegmentedElementType *s = new SegmentedElementType(create_type<float>());
    //Segments has field types MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPointerType pointing to:
    //SegmentedElementType has field types MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 3
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    //                          and MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 7
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    //                          and MPrimType with type code: 5
    //followed by
    //MPrimType with type code: 5
    //MPrimType with type code: 5
    SegmentsType *ss = new SegmentsType(create_type<float>());

    //ComparisonElementType has field types MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 3
    //                          and MPrimType with type code: 5
    //MPrimType with type code: 5
    //                          and MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 3
    //                          and MPrimType with type code: 5
    //MPrimType with type code: 5
    //                          and MPointerType pointing to:
    //MArrayType data type: MPointerType pointing to:
    //MPrimType with type code: 7
    //                          and MPrimType with type code: 5
    //MPrimType with type code: 5
    ComparisonElementType *c = new ComparisonElementType(create_type<float>());

//    t->dump();
//    f->dump();
//    e->dump();
    w->dump();
//    s->dump();
//    ss->dump();
//    c->dump();

    // { i8*, i32, i32 }
    t->codegen()->dump();
    // { { i8*, i32, i32 }* }
    f->codegen()->dump();
    // { { i8*, i32, i32 }*, { float*, i32, i32 }* }
    e->codegen()->dump();
    // { { { { i8*, i32, i32 }*, { float*, i32, i32 }* }**, i32, i32 }* }
    // the ** is the pointer to a bunch of ElementType pointers
    w->codegen()->dump();
    // { { i8*, i32, i32 }*, { float*, i32, i32 }*, i32 }
    s->codegen()->dump();
    // { { { { i8*, i32, i32 }*, { float*, i32, i32 }*, i32 }**, i32, i32 }* }
    ss->codegen()->dump();
    // { { i8*, i32, i32 }*, { i8*, i32, i32 }*, { float*, i32, i32 }* }
    c->codegen()->dump();

    return 0;
}
