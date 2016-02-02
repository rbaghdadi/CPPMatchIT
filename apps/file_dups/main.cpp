//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <openssl/md5.h>
#include "../../src/JIT.h"
#include "../../src/LLVM.h"
#include "../../src/Pipeline.h"
#include "../../src/TransformStage.h"
#include "../../src/MergeStages.h"
#include "../../src/FilterStage.h"
#include "../../src/ComparisonStage.h"
#include "./Stages.h"

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

    char *filepaths_file = av[1];
    std::cerr << "reading file paths from " << filepaths_file << std::endl;
    std::vector<const void *> filepaths;
    FILE *file = fopen(filepaths_file, "rt");
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        filepaths.push_back(strdup(line));
    }
//    fclose(file);
//    exit(8);
//
//    std::string fn = av[1];
//    std::ifstream file(fn, std::ifstream::in);
//    std::string line;
//    std::vector<const void *> filepaths;
//    while (std::getline(file, line)) {
//        const std::string s = line;
//        filepaths.push_back(strdup(s).c_str());
//        fprintf(stderr, "%s\n", last(filepaths));
//    }
//
//    file.close();
//
//    fprintf(stderr, "%s\n", filepaths[3]);

//    // generate some data
//    std::string str1 = "/Users/JRay/Desktop/IMG_1311.JPG";
//    std::string str2 = "/Users/JRay/Desktop/notes.txt";
//    std::string str3 = "/Users/JRay/Desktop/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf";
//
//    std::vector<const void *> data;
//    data.push_back(str1.c_str());
//    data.push_back(str1.c_str());
//    data.push_back(str1.c_str());
//    data.push_back(str1.c_str());
//    data.push_back(str1.c_str());
//    data.push_back(str2.c_str());
//    data.push_back(str1.c_str());
//    data.push_back(str1.c_str());
//    data.push_back(str1.c_str());
//    data.push_back(str2.c_str());
//    data.push_back(str3.c_str());
//    data.push_back(str2.c_str());
//    data.push_back(str2.c_str());

    /*
     * Create stages
     */

    // file filters
    Filter jpg_filt(llvm_filter, "llvm_filter", &jit);
    Filter txt_filt(txt_filter, "txt_filter", &jit);

    // transforms
    std::vector<MType *> filehash_field_types;
    MType *char_field = create_type<char *>();
    MType *uchar_field = create_type<unsigned char *>();
    filehash_field_types.push_back(char_field);
    filehash_field_types.push_back(uchar_field);
    Transform xform(transform, &jit, filehash_field_types);

    StructCheck scheck(struct_check, &jit, filehash_field_types);

    // comparison
    std::vector<MType *> comparison_field_types;
    MType *bool_field = create_type<bool>();
    comparison_field_types.push_back(bool_field);
    comparison_field_types.push_back(char_field);
    comparison_field_types.push_back(char_field);
    HashCompare hcomp(compare, &jit, filehash_field_types, comparison_field_types);

    /*
     * Create pipeline and run
     */

    Merge merged_jpg_filt_xform(&jit, &jpg_filt, &xform);
    Merge merged_jpg_filt_txt_filt(&jit, &jpg_filt, &txt_filt);

    Pipeline pipeline;
//    pipeline.register_stage(&jpg_filt);
//    pipeline.register_stage(&merged_jpg_filt_txt_filt);
    pipeline.register_stage(&merged_jpg_filt_xform);
    pipeline.register_stage(&scheck);
    pipeline.register_stage(&hcomp);
    pipeline.codegen(&jit, filepaths.size());
    jit.dump();
    jit.add_module();
    pipeline.simple_execute(&jit, &(filepaths[0]));

    return 0;

}
