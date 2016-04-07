//
// Created by Jessica Ray on 2/2/16.
//

#ifndef MATCHIT_STAGES_H
#define MATCHIT_STAGES_H

#include <string>
#include <vector>
#include "openssl/md5.h"
#include "../../src/Structures.h"
#include "../../src/ComparisonStage.h"
#include "../../src/FilterStage.h"
#include "../../src/TransformStage.h"

/*
 * Transforms
 */

class IdentityTransform : public TransformStage<File *, File *> {

public:

    IdentityTransform(File *(*transform)(File *), JIT *jit) :
            TransformStage(transform, "identity", jit, new FileType(), new FileType()) {}

};

extern "C" File *identity(File *file);

class Transform : public TransformStage<File *, Element<unsigned char> *> {

public:

    Transform(Element<unsigned char> *(*transform)(File *), JIT *jit) :
            TransformStage(transform, "transform", jit, new FileType, new ElementType(create_type<unsigned char>())) {}

};

extern "C" Element<unsigned char> *transform(File *file);

/*
 * Filters
 */
//
class Filter : public FilterStage<File *> {

public:

    Filter(bool (*filter)(File *), std::string transform_name, JIT *jit) : FilterStage(filter, transform_name, jit,
                                                                                       new FileType()) {}

};

extern "C" bool matlab_filter(File *file_path);

class CompFilter : public FilterStage<ComparisonElement<bool> *> {
public:
    CompFilter(bool (*filter)(ComparisonElement<bool> *), std::string transform_name, JIT *jit) :
            FilterStage(filter, transform_name, jit, new ComparisonElementType(create_type<bool>())) {}
};

extern "C" bool match_filter(ComparisonElement<bool> *comparison);

/*
 * Comparators
 */

class HashCompare : public ComparisonStage<Element<unsigned char> *, ComparisonElement<bool> *> {
public:
    HashCompare(ComparisonElement<bool> *(*compare)(Element<unsigned char> *, Element<unsigned char> *), JIT *jit) :
            ComparisonStage(compare, "compare", jit, new ElementType(create_type<unsigned char>()),
            new ComparisonElementType(create_type<bool>())) {}
};

extern "C" ComparisonElement<bool> *compare(Element<unsigned char> *file1, Element<unsigned char> *file2);




//
///*
// * User-defined datatypes
// */
//
//typedef struct {
//    char *file_path;
//    unsigned char *md5_hash;
//} FileHash;
//
//typedef struct {
//    bool is_match;
//    char *file_path1;
//    char *file_path2;
//} Comparison;
//
///*
// * Stage class definitions
// */
//
//// Transforms
//
////class IdentityTransform : public TransformStage<char *, char *> {
////public:
////    IdentityTransform(char * (*transform)(char *), JIT *jit) :
////            TransformStage(transform, "identity", jit) {}
////};
//
//class IdentityTransform : public TransformStage<char *, char *> {
//public:
//    IdentityTransform(ElementPtr<char> *(*transform)(ElementPtr<char> *), JIT *jit) :
//            TransformStage(transform, "identity", jit) {}
//};
//
//class Transform : public TransformStage<char *, FileHash *> {
//public:
//    // TODO this isn't very nice for the user to have to write
//    Transform(FileHash *(*transform)(char *), JIT *jit, std::vector<MType *> filehash_fields) :
//            TransformStage(transform, "transform", jit, std::vector<MType *>(), filehash_fields) {}
//};
//
//class StructCheck : public TransformStage<FileHash *, FileHash *> {
//public:
//    StructCheck(FileHash *(*transform)(FileHash *), JIT *jit, std::vector<MType *> filehash_fields) :
//            TransformStage(transform, "struct_check", jit, filehash_fields, filehash_fields) {}
//};
//
//// Filters
//
//class Filter : public FilterStage<char *> {
//public:
//    Filter(bool (*filter)(char*), std::string transform_name, JIT *jit) : FilterStage(filter, transform_name, jit) {}
//};
//
//class CompFilter : public FilterStage<Comparison *> {
//public:
//    CompFilter(bool (*filter)(Comparison*), std::string transform_name, JIT *jit, std::vector<MType *> comparison_fields) :
//            FilterStage(filter, transform_name, jit, comparison_fields) {}
//};
//
//// Comparators
//
//class HashCompare : public ComparisonStage<FileHash *, Comparison *> {
//public:
//    HashCompare(Comparison *(*compare)(FileHash *, FileHash *), JIT *jit, std::vector<MType *> filehash_fields,
//                std::vector<MType *> comparison_fields) : ComparisonStage(compare, "compare", jit, filehash_fields, comparison_fields) {}
//};
//
//
///*
// * Stage computations
// */
//
////extern "C" char *identity(char *file_path);
//extern "C" ElementPtr<char> *identity(ElementPtr<char> *file_path);
//
//extern "C" FileHash *struct_check(FileHash *in);
//
//// TODO need to free this stuff
//extern "C" FileHash* transform(char *file_path);
//
//extern "C" bool llvm_filter(char *file_path);
//
//extern "C" bool txt_filter(char *file_path);
//
//extern "C" bool match_filter(Comparison *comparison);
//
//extern "C" Comparison *compare(FileHash *file1, FileHash *file2);

#endif //MATCHIT_STAGES_H
