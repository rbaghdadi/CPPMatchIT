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
#include "../../src/CompositeTypes.h"
#include "./Stages.h"
#include "./paths.h"

// Basic structure of a program:
// 1. User extends necessary block classes
// 2. User creates the necessary "apply" functions for each extended block
// 3. User adds blocks to the pipeline
// 4. User points to the actual input data
// 5. User JITs the code


// 81 chars in the string

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
    for (int i = 0; i < 148; i++) {
        std::string s = paths[i];
        char dest[s.size() + 1];
        strncpy(dest, s.c_str(), s.size());
        dest[s.size()] = '\0';
        File *mfile = new File(s.size() + 1);
        mfile->add(dest, s.size() + 1);
        files.push_back(mfile);
    }

//     Now I have my File objects
    Transform xform(transform, &jit);
    Pipeline pipeline;
    pipeline.register_stage(&xform);
    pipeline.codegen(&jit, files.size());
    jit.dump();
    jit.add_module();
    pipeline.simple_execute(&jit,&(files[0]));

//
//    char *filepaths_file = av[1];
//    FILE *file = fopen(filepaths_file, "rb");
//    char line[256];
//    while (fgets(line, sizeof(line), file)) {
//        line[strcspn(line, "\n")] = 0;
//        char *split = strtok(line, "#");
//        int idx = 0;
//        unsigned int size;
//        char *path;
//        while (split != NULL) {
//            if (idx == 0) {
//                path = split;
//            } else if (idx == 1) {
//                size = atoi(split);
//            }
//            idx++;
//        }
//        std::cerr << path << " " << size << std::endl;
//

        // find the length of this input string
//        int cur_size = 0;
//        char cur_char = line[0];
//        while (cur_char != '\0') {
//            cur_size++;
//            cur_char = line[cur_size];
//        }
//        char substrline[cur_size];
//        for (int i = 0; i < cur_size; i++) {
//            substrline[i] = line[i];
//            std::cerr << line[i] << std::endl;
//        }
//
//        std::string s(line);
//        const char *x = s.substr(0, cur_size).c_str();
//
//        File *mfile = new File();
//        mfile->filepath = new MArray<char>(cur_size);
//        mfile->filepath->add(x, cur_size);
//        files.push_back(mfile);
//
//    for (int i = 0; i < files.size(); i++) {
//        std::cerr << files[i]->filepath->get_underlying_array() << std::endl;
//        std::cerr << files[i]->filepath->get_size() << std::endl;
//        std::cerr << files[i]->filepath->get_cur_idx() << std::endl;
//    }

//    for (int i = 0; i < input.inputs.get_cur_idx(); i++) {
//        std::cerr << input.inputs.get_underlying_array()->filepath.get_underlying_array() << std::endl;
//    }



//        ElementPtr<char> *mvecptr = (ElementPtr<char>*)malloc(sizeof(*mvecptr));
//        mvecptr->data = (char *)malloc(sizeof(mvecptr->data) * cur_size);
//        std::string s(line);
//        strcpy(mvecptr->data, s.substr(0, cur_size).c_str());
//        mvecptr->size = (size_t *)malloc(sizeof(mvecptr->size));
//        mvecptr->size[0] = cur_size;
//        inputs.push_back(mvecptr);
//    }

//
//    std::vector<ElementPtr<char> *> inputs;
//
//    char *filepaths_file = av[1];
//    std::cerr << "reading file paths from " << filepaths_file << std::endl;
//    std::vector<const void *> filepaths;
//    FILE *file = fopen(filepaths_file, "rb");
//    char line[256];
//    while (fgets(line, sizeof(line), file)) {
//        line[strcspn(line, "\n")] = 0;
//        int cur_size = 0; // find the length of this input string
//        char cur_char = line[0];
//        while (cur_char != '\0') {
//            cur_size++;
//            cur_char = line[cur_size];
//        }
//        ElementPtr<char> *mvecptr = (ElementPtr<char>*)malloc(sizeof(*mvecptr));
//        mvecptr->data = (char *)malloc(sizeof(mvecptr->data) * cur_size);
//        std::string s(line);
//        strcpy(mvecptr->data, s.substr(0, cur_size).c_str());
//        mvecptr->size = (size_t *)malloc(sizeof(mvecptr->size));
//        mvecptr->size[0] = cur_size;
//        inputs.push_back(mvecptr);
//    }
//
//    for (std::vector<ElementPtr<char> *>::iterator iter = inputs.begin(); iter != inputs.end(); iter++) {
//        std::cerr << (*iter)->data << " " << (*iter)->size[0] << std::endl;
//    }





    /*
     * Create stages
     */

    // transforms
//    std::vector<MType *> filehash_field_types;
//    MType *char_field = create_type<char *>();
//    MType *uchar_field = create_type<unsigned char *>();
//    filehash_field_types.push_back(char_field);
//    filehash_field_types.push_back(uchar_field);
//    Transform xform(transform, &jit, filehash_field_types);
//    StructCheck scheck(struct_check, &jit, filehash_field_types);

    // comparison
//    std::vector<MType *> comparison_field_types;
//    MType *bool_field = create_type<bool>();
//    comparison_field_types.push_back(bool_field);
//    comparison_field_types.push_back(char_field);
//    comparison_field_types.push_back(char_field);
//    HashCompare hcomp(compare, &jit, filehash_field_types, comparison_field_types);


    // file filters
//    Filter llvm_filt(llvm_filter, "llvm_filter", &jit);
//    Filter txt_filt(txt_filter, "txt_filter", &jit);
//    CompFilter res_filt(match_filter, "match_filter", &jit, comparison_field_types);

    /*
     * Create pipeline and run
     */

//    Merge merged_llvm_filt_xform(&jit, &llvm_filt, &xform);
//    Merge merged_jpg_filt_txt_filt(&jit, &llvm_filt, &txt_filt);

//    Pipeline pipeline;
//    pipeline.register_stage(&merged_llvm_filt_xform);
//    pipeline.register_stage(&llvm_filt);
//    pipeline.register_stage(&xform);
//    pipeline.register_stage(&scheck);
//    pipeline.register_stage(&hcomp);
//    pipeline.register_stage(&res_filt);
//    pipeline.codegen(&jit, filepaths.size());
//    jit.dump();
//    jit.add_module();
//    pipeline.simple_execute(&jit, &(filepaths[0]));

    return 0;

}
