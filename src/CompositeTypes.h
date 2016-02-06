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

    T *data; // 8 bytes
    int num_elements; // 4 bytes
    int cur_idx; // 4 bytes

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

// access the size field
llvm::LoadInst *codegen_marray_size_field(JIT *jit, llvm::LoadInst *loaded_marray) {
    std::vector<llvm::Value *> gep_size_field_idx;
    gep_size_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_size_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    llvm::Value *gep_size_field = jit->get_builder().CreateInBoundsGEP(loaded_marray, gep_size_field_idx);
    llvm::LoadInst *size_field = jit->get_builder().CreateLoad(gep_size_field);
    size_field->setAlignment(8);
    return size_field;
}

llvm::Value *codegen_marray_size_in_bytes(JIT *jit, llvm::LoadInst *loaded_marray, llvm::Value *data_type_size) {
    llvm::LoadInst *size_field = codegen_marray_size_field(jit, loaded_marray);
    llvm::Value *mul = jit->get_builder().CreateMul(size_field, data_type_size);
    llvm::Value *final_size = jit->get_builder().CreateAdd(mul, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 16)); // add on 16 for size of MArray
    return final_size;
}

}

/*
 * Data types available for the user
 */

// TODO restrict T so that it has to be one of my primitive types (and maybe a complex type)
// TODO there is a lot of mixing of templates and mtypes here and in other parts of the code (like the functions that generate code for each of these)
// TODO should this just be a struct like the rest to make thing easy?
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
struct Segments : BaseElement {
    MArray<SegmentedElement<T> *> *segments;
};

template <typename T>
struct ComparisonElement : BaseElement {
    MArray<char> *filepath1;
    MArray<char> *filepath2;
    // This could just be a single boolean--should that be a different struct or should they just store a single element bool MArray?
    MArray<T> *comparison;
};

//template <typename T>
//struct SegmentedComparisonElement : BaseElement {
//    MArray<char> *filepath1;
//    MArray<char> *filepath2;
//    MArray<T> *data1;
//    MArray<T> *data2;
//    unsigned int *offset1;
//    unsigned int *offset2;
//    bool is_match;
//};

/*
 * Bucketed versions
 */

//template <typename T>
//struct BucketedElement : BaseElement {
//    MArray<char> filepath;
//    MArray<T> data;
//    int bucket;
//};
//
//template <typename T>
//struct BucketedSegmentedElement : BaseElement {
//    MArray<char> filepath;
//    MArray<T> data;
//    unsigned int offset;
//    int bucket;
//};
//
//template <typename T>
//struct BucketedComparisonElement : BaseElement {
//    MArray<char> filepath1;
//    MArray<char> filepath2;
//    MArray<T> data1;
//    MArray<T> data2;
//    int bucket1;
//    int bucket2;
//    bool is_match;
//};
//
//template <typename T>
//struct BucketedSegmentedComparisonElement : BaseElement {
//    MArray<char> filepath1;
//    MArray<char> filepath2;
//    MArray<T> data1;
//    MArray<T> data2;
//    unsigned int offset1;
//    unsigned int offset2;
//    int bucket1;
//    int bucket2;
//    bool is_match;
//};

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
        // filepath
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        // data
        element_field_types.push_back(create_struct_reference_type(marray_user_field_types));
        // create our final type
        MPointerType *ptr = create_struct_reference_type(mtype_element, element_field_types);
        return ptr;
    }
};

template <typename T>
struct create_type<Segments<T> *> {
    operator MPointerType *() {
        // create the SegmentedElement
        MType *segmented = create_type<SegmentedElement<T> *>();
        // types in the SegmentedElement marray class
        MType *int_field = create_type<int>();
        std::vector<MType *> marray_seg_field_types;
        marray_seg_field_types.push_back(new MPointerType(segmented));
        marray_seg_field_types.push_back(int_field);
        marray_seg_field_types.push_back(int_field);
        // create the Segments
        std::vector<MType *> element_field_types;
//        MPointerType *p = new MPointerType(create_struct_reference_type(marray_seg_field_types));
        element_field_types.push_back(create_struct_reference_type(marray_seg_field_types));//create_type<MPointerType<create_struct_reference_type(marray_seg_field_types)> *>());
        // create our final type
        MPointerType *ptr = create_struct_reference_type(mtype_segments, element_field_types);
        return ptr;
    }
};

template <typename T>
struct create_type<ComparisonElement<T> *> {
    operator MPointerType *() {
        MType *user_type = create_type<T *>();
        MType *int_field = create_type<int>();
        MType *char_field = create_type<char *>();
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
        // filepath1
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        // filepath2
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        // comparison
        element_field_types.push_back(create_struct_reference_type(marray_user_field_types));
        // create our final type
        MPointerType *ptr = create_struct_reference_type(mtype_comparison_element, element_field_types);
        return ptr;
    }
};

template <typename T>
struct create_type<SegmentedElement<T> *> {
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
        // filepath
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        // data
        element_field_types.push_back(create_struct_reference_type(marray_user_field_types));
        // offset
        element_field_types.push_back(int_field);
        // create our final type
        MPointerType *ptr = create_struct_reference_type(mtype_segmented_element, element_field_types);
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

template <typename T>
struct mtype_of<Segments<T> > {
    operator mtype_code_t() {
        return mtype_segments;
    }
};

template <typename T>
struct mtype_of<SegmentedElement<T> > {
    operator mtype_code_t() {
        return mtype_segmented_element;
    }
};

template <>
struct mtype_of<File> {
    operator mtype_code_t() {
        return mtype_file;
    }
};

#endif //MATCHIT_COMPOSITETYPES_H
