//
// Created by Jessica Ray on 2/2/16.
//

#include <iostream>
#include "./Stages.h"

extern "C" Element<unsigned char> *identity(Element<unsigned char> *file) {
//    std::cerr << "Running function identity on " << file->filepath->get_underlying_array() << std::endl;
//    std::cerr << "Element size is  " << file->data->get_num_elements() << std::endl;
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i * 2], "%02x", file->data->get_underlying_array()[i]);
    }
    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
    return file;
}


//extern "C" Element<unsigned char>* transform(File *file_path) {
extern "C" Element<unsigned char>* transform(File *file_path) {
    std::cerr << "Transform " << file_path->get_underlying_array();
    FILE *file = fopen(file_path->get_underlying_array(), "rb");
    MD5_CTX md5_context;
    int bytes_read;
    unsigned char buffer[1024];
    // our output type
    Element<unsigned char> *md5_hash = (Element<unsigned char> *) malloc(sizeof(Element<unsigned char>));
    // set space for the data in the output type
    md5_hash->filepath = new MArray<char>(file_path->get_num_elements());
    md5_hash->data = new MArray<unsigned char>(MD5_DIGEST_LENGTH);
    // http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c
    MD5_Init(&md5_context);
    while ((bytes_read = fread(buffer, 1, 1024, file)) != 0) {
        MD5_Update(&md5_context, buffer, bytes_read);
    }
    MD5_Final(md5_hash->data->get_underlying_array(), &md5_context);
    md5_hash->filepath->add(file_path->get_underlying_array(), file_path->get_num_elements());
    fclose(file);
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i * 2], "%02x", md5_hash->data->get_underlying_array()[i]);
    }
    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
    return md5_hash;
}

extern "C" bool matlab_filter(File *file_path) {
    // 0 => not in string, so we can keep it
    std::cerr << "Running llvm_filter: " << file_path->get_underlying_array() << " ";
    char *cmp = strstr(file_path->get_underlying_array(), ".m");
    if (cmp == NULL) {
        std::cerr << "Keep me" << std::endl;
        return true;
    } else {
        std::cerr << "Drop me" << std::endl;
        return false;
    }
}

extern "C" ComparisonElement<bool> *compare(Element<unsigned char> *file1, Element<unsigned char> *file2) {
    std::cerr << "Comparing: " << file1->filepath->get_underlying_array() << " and " << file2->filepath->get_underlying_array();
    bool is_match = true;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (file1->data->get_underlying_array()[i] != file2->data->get_underlying_array()[i]) {
            is_match = false;
            break;
        }
    }
    ComparisonElement<bool> *comparison = (ComparisonElement<bool> *) malloc(sizeof(*comparison));
    comparison->filepath1 = new MArray<char>(file1->filepath->get_num_elements());
    comparison->filepath2 = new MArray<char>(file2->filepath->get_num_elements());
    comparison->filepath1->add(file1->filepath->get_underlying_array(), file1->filepath->get_num_elements());
    comparison->filepath2->add(file2->filepath->get_underlying_array(), file2->filepath->get_num_elements());
    comparison->comparison = new MArray<bool>(1);
    comparison->comparison->add(is_match);
    std::cerr << " is match?: " << is_match << std::endl;
    return comparison;
}

extern "C" bool match_filter(ComparisonElement<bool> *comparison) {
    if (comparison->comparison->get_underlying_array()[0]) {
        std::cerr << "Final: " << comparison->filepath1->get_underlying_array() << " ### " << comparison->filepath2->get_underlying_array() << std::endl;
        return true;
    } else {
        return false;
    }
}