//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include <stdlib.h>
#include <openssl/md5.h>
//#include "./ComparisonBlock.h"
//#include "./FilterBlock.h"
#include "./JIT.h"
#include "./LLVM.h"
#include "./Pipeline.h"
#include "./TransformStage.h"
#include "./MergeStages.h"

// Basic structure of a program:
// 1. User extends necessary block classes
// 2. User creates the necessary "apply" functions for each extended block
// 3. User adds blocks to the pipeline
// 4. User points to the actual input data
// 5. User JITs the code

typedef struct {
    char *file_path;
    unsigned char *md5_hash;
} FileHash;

typedef struct {
    char *file_path1;
    char *file_path2;
    bool is_match;
} Comparison;

/*
 * Stage class definitions
 */

class IdentityTransform : public TransformStage<char *, char *> {
public:
    IdentityTransform(char * (*transform)(char *), JIT *jit) :
            TransformStage(transform, "identity", jit) {}
};

class Transform : public TransformStage<char *, FileHash *> {
public:
    // TODO this isn't very nice for the user to have to write
    Transform(FileHash *(*transform)(char *), JIT *jit, std::vector<MType *> filehash_fields) :
            TransformStage(transform, "transform", jit, std::vector<MType *>(), filehash_fields) {}
};

//class Filter : public FilterBlock<char *> {
//public:
//    Filter(bool (*filter)(char*), std::string transform_name, JIT *jit) : FilterBlock(filter, transform_name, jit) {}
//};
//
//class HashCompare : public ComparisonBlock<FileHash *, Comparison *> {
//public:
//    HashCompare(Comparison *(*compare)(FileHash *, FileHash *), JIT *jit, std::vector<MType *> filehash_fields,
//                std::vector<MType *> comparison_fields) : ComparisonBlock(compare, "compare", jit, filehash_fields, comparison_fields) {}
//};
//
class StructCheck : public TransformStage<FileHash *, FileHash *> {
public:
    StructCheck(FileHash *(*transform)(FileHash *), JIT *jit, std::vector<MType *> filehash_fields) :
            TransformStage(transform, "struct_check", jit, filehash_fields, filehash_fields) {}
};

/*
 * Stage "apply" functions
 */

extern "C" FileHash *struct_check(FileHash *in) {
    std::cerr << "In struct_check. File path is " << in->file_path << std::endl;
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i * 2], "%02x", (unsigned int) in->md5_hash[i]);
    }
    fprintf(stderr, "md5 digest: %s\n", md5_str);
    return in;
}

extern "C" char *identity(char *file_path) {
    std::cerr << "Running function identity on " << file_path << std::endl;
    return file_path;
}

// TODO need to free this stuff
extern "C" FileHash* transform(char *file_path) {
    std::cerr << "Transform " << file_path;
    FILE *file = fopen(file_path, "rt");
    MD5_CTX md5_context;
    int bytes_read;
    unsigned char buffer[1024];

    FileHash *md5_hash = (FileHash *) malloc(sizeof(FileHash));
    md5_hash->file_path = (char *) malloc(sizeof(char) * 150);
    md5_hash->md5_hash = (unsigned char *) malloc(sizeof(unsigned char) * MD5_DIGEST_LENGTH);
    // http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c
    MD5_Init(&md5_context);
    while ((bytes_read = fread(buffer, 1, 1024, file)) != 0) {
        MD5_Update(&md5_context, buffer, bytes_read);
    }
    MD5_Final(md5_hash->md5_hash, &md5_context);
    strcpy(md5_hash->file_path, file_path);
    fclose(file);
    // http://www.askyb.com/cpp/openssl-md5-hashing-example-in-cpp/
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i * 2], "%02x", (unsigned int) md5_hash->md5_hash[i]);
    }
    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
    return md5_hash;
}

// TODO should probably check that we are only looking at the suffix of a file (so we would have to find where the \0 termination characters start and then look 4 back)
extern "C" bool jpg_filter(char *file_path) {
    // 0 => not in string, so we can keep it
    std::cerr << "Running jpg_filter: " << file_path << " ";//std::endl; //<< "    " << strstr(file_path, ".JPG") << std::endl;
    char *cmp = strstr(file_path, ".JPG");
    if (cmp == NULL) {
        std::cerr << "Keep me" << std::endl;
        return true;
    } else {
        std::cerr << "Drop me" << std::endl;
        return false;
    }
}

extern "C" bool txt_filter(char *file_path) {
    // 0 => not in string, so we can keep it
    std::cerr << "Running txt_filter: " << file_path << " ";//std::endl; //<< "    " << strstr(file_path, ".JPG") << std::endl;
    char *cmp = strstr(file_path, ".txt");
    if (cmp == NULL) {
        std::cerr << "Keep me" << std::endl;
        return true;
    } else {
        std::cerr << "Drop me" << std::endl;
        return false;
    }
}

extern "C" Comparison *compare(FileHash *file1, FileHash *file2) {
    std::cerr << "Comparing: " << file1->file_path << " and " << file2->file_path;
    bool is_match = true;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (file1->md5_hash[i] != file2->md5_hash[i]) {
            is_match = false;
            break;
        }
    }
    Comparison *comparison = (Comparison *) malloc(sizeof(Comparison));
    comparison->file_path1 = (char *) malloc(sizeof(char) * 50);
    comparison->file_path2 = (char *) malloc(sizeof(char) * 50);
    comparison->is_match = is_match;
    strcpy(comparison->file_path1, file1->file_path);
    strcpy(comparison->file_path2, file2->file_path);
    std::cerr << " is match?: " << is_match << std::endl;

    return comparison;
}

int main() {

    /*
     * Init LLVM and setup the JIT
     */

    LLVM::init();
    JIT jit;

    // generate some data
    std::string str1 = "/Users/JRay/Desktop/IMG_1311.JPG";
    std::string str2 = "/Users/JRay/Desktop/notes.txt";

    std::vector<const void *> data;
    data.push_back(str1.c_str());
    data.push_back(str1.c_str());
    data.push_back(str1.c_str());
    data.push_back(str1.c_str());
    data.push_back(str1.c_str());
    data.push_back(str1.c_str());
    data.push_back(str1.c_str());
    data.push_back(str1.c_str());
    data.push_back(str2.c_str());
    data.push_back(str2.c_str());
    data.push_back(str2.c_str());

    /*
     * Create blocks
     */

//    // create the file jpg_filter
//    Filter jpg_filt(jpg_filter, "jpg_filter", &jit);
//    jpg_filt.codegen();
//    Filter txt_filt(txt_filter, "txt_filter", &jit);
//    txt_filt.codegen();

    // fake transform
//    IdentityTransform identity_xform(identity, &jit);
//    identity_xform.codegen();

    // create the transform
    std::vector<MType *> filehash_field_types;
    MType *char_field = create_type<char *>();
    MType *uchar_field = create_type<unsigned char *>();
    filehash_field_types.push_back(char_field);
    filehash_field_types.push_back(uchar_field);

    Transform xform(transform, &jit, filehash_field_types);
//    xform.codegen();

//    // create the comparison
//    std::vector<MType *> comparison_field_types;
//    MType *bool_field = create_type<bool *>();
//    comparison_field_types.push_back(bool_field);
//    comparison_field_types.push_back(char_field);
//    comparison_field_types.push_back(char_field);
//
//    HashCompare hcomp(compare, &jit, filehash_field_types, comparison_field_types);
//    hcomp.codegen();
//
    StructCheck scheck(struct_check, &jit, filehash_field_types);
//    scheck.codegen();
//
//    /*
//     * Create pipeline and run
//     */
//
//    // create the pipeline
//    // TODO have the pipeline take in an initial data size so it's not hardcoded to be BUFFER_SIZE, and have it return
//    // the final number of elements left when it is done processing. That way we can hook together a bunch of pipelines and such
    Pipeline pipeline;
//    pipeline.register_block(&identity_xform);
//    pipeline.register_block(&txt_filt);
//    pipeline.register_block(&identity_xform);
//    pipeline.register_block(&xform);
//    pipeline.register_block(&scheck);
//    pipeline.register_block(&scheck);
//    pipeline.register_block(&hcomp);
//

    ImpureStage s = Opt::merge_stages_again(&jit, &xform, &scheck);
    pipeline.register_stage(&s);
    pipeline.codegen(jit);
    jit.dump();
    jit.add_module();
    pipeline.simple_execute(jit, &(data[0]));

    return 0;

}
