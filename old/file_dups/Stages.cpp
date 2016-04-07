//
// Created by Jessica Ray on 2/2/16.
//

#include <iostream>
#include "./Stages.h"

extern "C" File * identity(File *file) {
//    char md5_str[33];
//    for (int i = 0; i < 16; i++) { // file.get_data(i)
//        sprintf(&md5_str[i * 2], "%02x", file->get_data()->get_array()[i]);
//    }
//    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
    return file;
}

extern "C" Element<unsigned char> *transform(File *file_path) {
    std::cerr << "Transform " << file_path->get_filepath()->get_array();
    FILE *file = fopen(file_path->get_filepath()->get_array(), "rb");
    MD5_CTX md5_context;
    int bytes_read;
    unsigned char buffer[1024];
    // our output type
    Element<unsigned char> *md5_hash = (Element<unsigned char> *) malloc(sizeof(Element<unsigned char>));
    MArray<char> *new_filepath = new MArray<char>(file_path->get_filepath()->get_array(), file_path->get_filepath()->get_malloc_size());
    md5_hash->set_filepath(new_filepath);
    MArray<unsigned char> *hash = new MArray<unsigned char>(MD5_DIGEST_LENGTH);
    md5_hash->set_data(hash);
    MD5_Init(&md5_context);
    while ((bytes_read = fread(buffer, 1, 1024, file)) != 0) {
        MD5_Update(&md5_context, buffer, bytes_read);
    }
    MD5_Final(md5_hash->get_data()->get_array(), &md5_context);
    fclose(file);
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i * 2], "%02x", md5_hash->get_data()->get_array()[i]);
    }
    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
    return md5_hash;
}

extern "C" bool matlab_filter(File *file_path) {
    // 0 => not in string, so we can keep it
    std::cerr << "Running llvm_filter: " << file_path->get_filepath()->get_array() << " ";
    char *cmp = strstr(file_path->get_filepath()->get_array(), ".m");
    if (cmp == NULL) {
        std::cerr << "Keep me" << std::endl;
        return true;
    } else {
        std::cerr << "Drop me" << std::endl;
        return false;
    }
}

extern "C" ComparisonElement<bool> *compare(Element<unsigned char> *file1, Element<unsigned char> *file2) {
    std::cerr << "Comparing: " << file1->get_filepath()->get_array() << " and " << file2->get_filepath()->get_array();
    bool is_match = true;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (file1->get_data()->get_array()[i] != file2->get_data()->get_array()[i]) {
            is_match = false;
            break;
        }
    }
    ComparisonElement<bool> *comparison = (ComparisonElement<bool> *) malloc(sizeof(*comparison));
    MArray<char> *new_filepath1 = new MArray<char>(file1->get_filepath()->get_array(), file1->get_filepath()->get_malloc_size());
    MArray<char> *new_filepath2 = new MArray<char>(file2->get_filepath()->get_array(), file2->get_filepath()->get_malloc_size());
    MArray<bool> *comp = new MArray<bool>(1);
    comparison->set_filepath1(new_filepath1);
    comparison->set_filepath2(new_filepath2);
    comparison->set_comparison(comp);
    comparison->get_comparison()->add(is_match);
    std::cerr << " is match?: " << is_match << std::endl;
    return comparison;
}

extern "C" bool match_filter(ComparisonElement<bool> *comparison) {
    if (comparison->get_comparison()->get_array()[0]) {
        std::cerr << "Final: " << comparison->get_filepath1()->get_array() << " ### " <<
        comparison->get_filepath2()->get_array() << std::endl;
        return true;
    } else {
        return false;
    }
}