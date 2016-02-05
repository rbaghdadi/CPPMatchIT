//
// Created by Jessica Ray on 2/2/16.
//

#ifndef MATCHIT_STAGES_H
#define MATCHIT_STAGES_H

#include <string>
#include <vector>
#include "openssl/md5.h"
#include "../../src/CompositeTypes.h"
#include "../../src/ComparisonStage.h"
#include "../../src/FilterStage.h"
#include "../../src/TransformStage.h"

/*
 * Transforms
 */

class IdentityTransform : public TransformStage<Element<unsigned char> *, Element<unsigned char> *> {

public:

    IdentityTransform(Element<unsigned char> *(*transform)(Element<unsigned char> *), JIT *jit) :
            TransformStage(transform, "identity", jit) {}

};

extern "C" Element<unsigned char> *identity(Element<unsigned char> *file);

class Transform : public TransformStage<File *, Element<unsigned char> *> {

public:

    Transform(Element<unsigned char> *(*transform)(File *), JIT *jit) : TransformStage(transform, "transform", jit) {}

};

extern "C" Element<unsigned char> *transform(File *file);

/*
 * Filters
 */

class Filter : public FilterStage<File *> {

public:

    Filter(bool (*filter)(File *), std::string transform_name, JIT *jit) : FilterStage(filter, transform_name, jit) {}

};

extern "C" bool llvm_filter(File *file_path);

class CompFilter : public FilterStage<ComparisonElement<unsigned char> *> {
public:
    CompFilter(bool (*filter)(ComparisonElement<unsigned char> *), std::string transform_name, JIT *jit) :
            FilterStage(filter, transform_name, jit) {}
};

extern "C" bool match_filter(ComparisonElement<unsigned char> *comparison);

/*
 * Comparators
 */

class HashCompare : public ComparisonStage<Element<unsigned char> *, ComparisonElement<unsigned char> *> {
public:
    HashCompare(ComparisonElement<unsigned char> *(*compare)(Element<unsigned char> *, Element<unsigned char> *), JIT *jit) :
            ComparisonStage(compare, "compare", jit) {}
};

extern "C" ComparisonElement<unsigned char> *compare(Element<unsigned char> *file1, Element<unsigned char> *file2);





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
