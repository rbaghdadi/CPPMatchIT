//
// Created by Jessica Ray on 2/2/16.
//

#include <iostream>
#include "./Stages.h"

extern "C" File *identity(File *file) {
    std::cerr << "Running function identity on " << file->get_underlying_array() << std::endl;
    return file;
}


extern "C" Element<unsigned char>* transform(File *file_path) {
    std::cerr << "Transform " << file_path->get_underlying_array();
    FILE *file = fopen(file_path->get_underlying_array(), "rb");
    MD5_CTX md5_context;
    int bytes_read;
    unsigned char buffer[1024];
    // our output type
    Element<unsigned char> *md5_hash = (Element<unsigned char> *) malloc(sizeof(*md5_hash));
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

extern "C" bool llvm_filter(File *file_path) {
    // 0 => not in string, so we can keep it
    std::cerr << "Running llvm_filter: " << file_path->get_underlying_array() << " ";
    char *cmp = strstr(file_path->get_underlying_array(), ".ll");
    if (cmp == NULL) {
        std::cerr << "Keep me" << std::endl;
        return true;
    } else {
        std::cerr << "Drop me" << std::endl;
        return false;
    }
}

extern "C" ComparisonElement<unsigned char> *compare(Element<unsigned char> *file1, Element<unsigned char> *file2) {
    std::cerr << "Comparing: " << file1->filepath->get_underlying_array() << " and " << file2->filepath->get_underlying_array();
    bool is_match = true;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (file1->data->get_underlying_array()[i] != file2->data->get_underlying_array()[i]) {
            is_match = false;
            break;
        }
    }
    ComparisonElement<unsigned char> *comparison = (ComparisonElement<unsigned char> *) malloc(sizeof(*comparison));
    comparison->data1 = new MArray<unsigned char>(file1->data->get_num_elements());
    comparison->data2 = new MArray<unsigned char>(file2->data->get_num_elements());
    comparison->filepath1 = new MArray<char>(file1->filepath->get_num_elements());
    comparison->filepath2 = new MArray<char>(file2->filepath->get_num_elements());
    comparison->data1->add(file1->data->get_underlying_array(), file1->data->get_num_elements());
    comparison->data2->add(file2->data->get_underlying_array(), file2->data->get_num_elements());
    comparison->filepath1->add(file1->filepath->get_underlying_array(), file1->filepath->get_num_elements());
    comparison->filepath2->add(file2->filepath->get_underlying_array(), file2->filepath->get_num_elements());
    comparison->is_match = is_match;
    std::cerr << " is match?: " << is_match << std::endl;
    return comparison;
}

extern "C" bool match_filter(ComparisonElement<unsigned char> *comparison) {
    if (comparison->is_match) {
        std::cerr << "Final: " << comparison->filepath1->get_underlying_array() << " ### " << comparison->filepath2->get_underlying_array() << std::endl;
        return true;
    } else {
        return false;
    }
}

//extern "C" FileHash *struct_check(FileHash *in) {
//    std::cerr << "In struct_check. File path is " << in->file_path << std::endl;
//    char md5_str[33];
//    for (int i = 0; i < 16; i++) {
//        sprintf(&md5_str[i * 2], "%02x", (unsigned char) in->md5_hash[i]);
//    }
//    fprintf(stderr, "md5 digest: %s\n", md5_str);
//    return in;
//}
//
//// TODO need to free this stuff
////extern "C" FileHash* transform(mvector<char> file_path) {
////    std::cerr << "Transform " << *file_path;
////    FILE *file = fopen(*file_path, "rb");
////    MD5_CTX md5_context;
////    int bytes_read;
////    unsigned char buffer[1024];
////
////    FileHash *md5_hash = (FileHash *) malloc(sizeof(FileHash));
//////    md5_hash->file_path = mvecify(char, file_path->size()); //(char *) malloc(sizeof(char) * 150);
//////    md5_hash->md5_hash = mvecify(unsigned char, MD5_DIGEST_LENGTH);//(unsigned char *) malloc(sizeof(unsigned char) * MD5_DIGEST_LENGTH);
////    // http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c
////    MD5_Init(&md5_context);
////    while ((bytes_read = fread(buffer, 1, 1024, file)) != 0) {
////        MD5_Update(&md5_context, buffer, bytes_read);
////    }
////    MD5_Final(as_array(md5_hash->md5_hash), &md5_context);
////    strcpy(md5_hash->file_path, file_path);
////
////    fclose(file);
////    char md5_str[33];
////    for (int i = 0; i < 16; i++) {
////        sprintf(&md5_str[i * 2], "%02x", as_array(md5_hash->md5_hash)[i]);
////    }
////    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
////    return md5_hash;
////}
//
//extern "C" FileHash* transform(char *file_path) {
//    std::cerr << "Transform " << file_path;
//    FILE *file = fopen(file_path, "rt");
//    MD5_CTX md5_context;
//    int bytes_read;
//    unsigned char buffer[1024];
//
//    FileHash *md5_hash = (FileHash *) malloc(sizeof(FileHash));
//    md5_hash->file_path = (char *) malloc(sizeof(char) * 150);
//    md5_hash->md5_hash = (unsigned char *) malloc(sizeof(unsigned char) * MD5_DIGEST_LENGTH);
//    // http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c
//    MD5_Init(&md5_context);
//    while ((bytes_read = fread(buffer, 1, 1024, file)) != 0) {
//        MD5_Update(&md5_context, buffer, bytes_read);
//    }
//    MD5_Final(md5_hash->md5_hash, &md5_context);
//    strcpy(md5_hash->file_path, file_path);
//    fclose(file);
//    // http://www.askyb.com/cpp/openssl-md5-hashing-example-in-cpp/
//    char md5_str[33];
//    for (int i = 0; i < 16; i++) {
//        sprintf(&md5_str[i * 2], "%02x", (unsigned int) md5_hash->md5_hash[i]);
//    }
//    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
//    return md5_hash;
//}
//
//extern "C" bool llvm_filter(char *file_path) {
//    // 0 => not in string, so we can keep it
//    std::cerr << "Running jpg_filter: " << file_path << " ";
//    char *cmp = strstr(file_path, ".ll");
//    if (cmp == NULL) {
//        std::cerr << "Keep me" << std::endl;
//        return true;
//    } else {
//        std::cerr << "Drop me" << std::endl;
//        return false;
//    }
//}
//
//extern "C" bool txt_filter(char *file_path) {
//    // 0 => not in string, so we can keep it
//    std::cerr << "Running txt_filter: " << file_path << " ";
//    char *cmp = strstr(file_path, ".txt");
//    if (cmp == NULL) {
//        std::cerr << "Keep me" << std::endl;
//        return true;
//    } else {
//        std::cerr << "Drop me" << std::endl;
//        return false;
//    }
//}
//
//extern "C" bool match_filter(Comparison *comparison) {
//    if (comparison->is_match) {
//        std::cerr << "Final: " << comparison->file_path1 << " ### " << comparison->file_path2 << std::endl;
//        return true;
//    } else {
//        return false;
//    }
//}
//
//extern "C" Comparison *compare(FileHash *file1, FileHash *file2) {
//    std::cerr << "Comparing: " << file1->file_path << " and " << file2->file_path;
//    bool is_match = true;
//    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
//        if (file1->md5_hash[i] != file2->md5_hash[i]) {
//            is_match = false;
//            break;
//        }
//    }
//    Comparison *comparison = (Comparison *) malloc(sizeof(Comparison));
//    comparison->file_path1 = (char *) malloc(sizeof(char) * 256);
//    comparison->file_path2 = (char *) malloc(sizeof(char) * 256);
//    comparison->is_match = is_match;
//    strcpy(comparison->file_path1, file1->file_path);
//    strcpy(comparison->file_path2, file2->file_path);
//    std::cerr << " is match?: " << is_match << std::endl;
//    return comparison;
//}