//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_COMPOSITETYPES_H
#define MATCHIT_COMPOSITETYPES_H

#include <stdlib.h>
#include <assert.h>
#include "llvm/IR/Value.h"
#include "./JIT.h"
#include "./CodegenUtils.h"
#include "./MType.h"


// use this instead of an array
template <typename T>
class MArray {

private:

    T *data;
    int num_elements;
    int cur_idx;

public:

    MArray() : data(nullptr), num_elements(0), cur_idx(0) { }

    MArray(int num_elements) : data(nullptr), cur_idx(0) {
        mmalloc(num_elements);
    }

    void mmalloc(int num_elements) {
        assert(!data);
        this->num_elements = num_elements;
        data = (T*)malloc(sizeof(T) * num_elements);
    }

    void mfree() {
        if (data) {
            free(data);
            num_elements = 0;
            cur_idx = 0;
            data = nullptr;
        }
    }

    void mrealloc(int new_size) {
        data = (T*)realloc(data, new_size);
    }

    void add(T element) {
        assert(data);
        assert(cur_idx != num_elements);
        data[cur_idx++] = element;
    }

    // TODO use a memcpy instead
    void add(const T* elements, int num_elements) {
        if (!data) {
            mmalloc(num_elements);
        } else if (cur_idx + num_elements > num_elements) {
            mrealloc(cur_idx + num_elements);
        }
        for (int i = 0; i < num_elements; i++) {
            data[cur_idx++] = elements[i];
        }
    }

    int get_num_elements() {
        return num_elements;
    }

    int get_cur_idx() {
        return cur_idx;
    }

    T* get_underlying_array() {
        return data;
    }

};

namespace {
//llvm::Value *codegen_marray_size(JIT *jit, llvm::LoadInst *loaded_marray, llvm::Value *data_type_size) {
//    std::vector<llvm::Value *> gep_size_field_idx;
//    gep_size_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//    llvm::Value *gep_size_field = jit->get_builder().CreateInBoundsGEP(loaded_marray, gep_size_field_idx);
//    llvm::LoadInst *size_field = jit->get_builder().CreateLoad(gep_size_field);
//    llvm::Value *final_size = jit->get_builder().CreateAdd(jit->get_builder().CreateMul(size_field, data_type_size),
//                                                           llvm::ConstantInt::get(
//                                                                   llvm::Type::getInt32Ty(llvm::getGlobalContext()),
//                                                                   16)); // add on 16 for size of MArray
//    return final_size;
//}

llvm::Value *codegen_marray_size_ptr(JIT *jit, llvm::LoadInst *loaded_marray, llvm::Value *data_type_size) {
    std::vector<llvm::Value *> gep_size_field_idx;
    gep_size_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_size_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    llvm::Value *gep_size_field = jit->get_builder().CreateInBoundsGEP(loaded_marray, gep_size_field_idx);
    llvm::LoadInst *size_field = jit->get_builder().CreateLoad(gep_size_field);
    size_field->setAlignment(8);
    llvm::Value *mul = jit->get_builder().CreateMul(size_field, data_type_size);
    llvm::Value *final_size = jit->get_builder().CreateAdd(mul, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 16)); // add on 16 for size of MArray
    CodegenUtils::codegen_fprintf_int(jit, size_field);
    return final_size;
}
}

/*
 * Data types available for the user
 */

// TODO restrict T so that it has to be one of my primitive types (and maybe a complex type)
typedef MArray<char> File;

struct BaseElement {
};

template <typename T>
struct Element : BaseElement {
    MArray<char> *filepath;
    MArray<T> *data;
};

template <typename T>
struct SegmentedElement : BaseElement {
    MArray<char> *filepath;
    MArray<T> *data;
    unsigned int offset;
};

template <typename T>
struct ComparisonElement : BaseElement {
    MArray<char> *filepath1;
    MArray<char> *filepath2;
    MArray<T> *data1;
    MArray<T> *data2;
    bool is_match;
};

template <typename T>
struct SegmentedComparisonElement : BaseElement {
    MArray<char> *filepath1;
    MArray<char> *filepath2;
    MArray<T> *data1;
    MArray<T> *data2;
    unsigned int *offset1;
    unsigned int *offset2;
    bool is_match;
};

/*
 * Bucketed versions
 */

template <typename T>
struct BucketedElement : BaseElement {
    MArray<char> filepath;
    MArray<T> data;
    int bucket;
};

template <typename T>
struct BucketedSegmentedElement : BaseElement {
    MArray<char> filepath;
    MArray<T> data;
    unsigned int offset;
    int bucket;
};

template <typename T>
struct BucketedComparisonElement : BaseElement {
    MArray<char> filepath1;
    MArray<char> filepath2;
    MArray<T> data1;
    MArray<T> data2;
    int bucket1;
    int bucket2;
    bool is_match;
};

template <typename T>
struct BucketedSegmentedComparisonElement : BaseElement {
    MArray<char> filepath1;
    MArray<char> filepath2;
    MArray<T> data1;
    MArray<T> data2;
    unsigned int offset1;
    unsigned int offset2;
    int bucket1;
    int bucket2;
    bool is_match;
};

// TODO need a delete somewhere for all of these
template <typename T>
struct create_type<Element<T> *> {
    operator MPointerType *() {
        MType *user_type = create_type<T *>();
        MType *int_field = create_type<int>();
        MType *char_field = create_type<char *>();
        // types in the filepath marray class
        std::vector<MType *> marray_char_field_types;
        // types in the user data marray class
        std::vector<MType *> marray_user_field_types;
        // types in the Element struct
        std::vector<MType *> element_field_types;
        // add the fields
        marray_char_field_types.push_back(char_field);
        marray_char_field_types.push_back(int_field);
        marray_char_field_types.push_back(int_field);
        marray_user_field_types.push_back(user_type);
        marray_user_field_types.push_back(int_field);
        marray_user_field_types.push_back(int_field);
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        element_field_types.push_back(create_struct_reference_type(marray_user_field_types));
        // create our final type
        MPointerType *ptr = create_struct_reference_type(mtype_element, element_field_types);
        return ptr;
    }
};

template <typename T>
struct create_type<ComparisonElement<T> *> {
    operator MPointerType *() {
        MType *user_type = create_type<T *>();
        MType *int_field = create_type<int>();
        MType *char_field = create_type<char *>();
        MType *bool_field = create_type<bool>();
        // types in the filepath marray class
        std::vector<MType *> marray_char_field_types;
        // types in the user data marray class
        std::vector<MType *> marray_user_field_types;
        // types in the ComparisonElement struct
        std::vector<MType *> element_field_types;
        // add the fields
        marray_char_field_types.push_back(char_field);
        marray_char_field_types.push_back(int_field);
        marray_char_field_types.push_back(int_field);
        marray_user_field_types.push_back(user_type);
        marray_user_field_types.push_back(int_field);
        marray_user_field_types.push_back(int_field);
        // duplicate each since there are two elements in a comparison
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        element_field_types.push_back(create_struct_reference_type(marray_user_field_types));
        element_field_types.push_back(create_struct_reference_type(marray_user_field_types));
        element_field_types.push_back(bool_field);
        // create our final type
        MPointerType *ptr = create_struct_reference_type(mtype_comparison_element, element_field_types);
        return ptr;
    }
};

template <>
struct create_type<File *> {
    operator MPointerType *() {
        MType *int_field = create_type<int>();
        MType *char_field = create_type<char *>();
        // types in the filepath
        std::vector<MType *> marray_char_field_types;
        marray_char_field_types.push_back(char_field);
        marray_char_field_types.push_back(int_field);
        marray_char_field_types.push_back(int_field);
        MPointerType *ptr = create_struct_reference_type(mtype_file, marray_char_field_types);
        return ptr;
    }
};

template <typename T>
struct mtype_of<Element<T> > {
    operator mtype_code_t() {
        return mtype_element;
    }
};

template <typename T>
struct mtype_of<ComparisonElement<T> > {
    operator mtype_code_t() {
        return mtype_comparison_element;
    }
};

template <>
struct mtype_of<File> {
    operator mtype_code_t() {
        return mtype_file;
    }
};


#endif //MATCHIT_COMPOSITETYPES_H
