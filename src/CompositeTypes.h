//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_COMPOSITETYPES_H
#define MATCHIT_COMPOSITETYPES_H

#include <stdlib.h>
#include <assert.h>
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

/*
 * Data types available for the user
 */

struct BaseElement {
    virtual std::vector<MType *> get_struct_fields() = 0;
};

//template <typename T>
//struct TestElement : BaseElement {
//private:
//
//public:
//
//    TestElement() {}
//    T element;
//
//
//
//};


//struct File : BaseElement {
//    MArray<char> *filepath;
//};

typedef MArray<char> File;

template <typename T>
struct Element : BaseElement {
    MArray<char> *filepath;
    MArray<T> *data;

};

template <typename T>
struct SegmentedElement : BaseElement {
    MArray<char> filepath;
    MArray<T> data;
    unsigned int offset;
};

template <typename T>
struct ComparisonElement : BaseElement {
    MArray<char> filepath1;
    MArray<char> filepath2;
    MArray<T> data1;
    MArray<T> data2;
};

template <typename T>
struct SegmentedComparisonElement : BaseElement {
    MArray<char> filepath1;
    MArray<char> filepath2;
    MArray<T> data1;
    MArray<T> data2;
    unsigned int offset1;
    unsigned int offset2;
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
};
//
///*
// * Initial input type
// */
////struct Input {
////    MArray<File> inputs;
////};
//



// TODO need a delete somewhere for all of these
template <typename T>
struct create_type<Element<T> *> {
    operator MPointerType*() {
        std::vector<MType *> marray_char_field_types;
        std::vector<MType *> marray_user_field_types;
        std::vector<MType *> element_field_types;
        MType *user_type = create_type<T *>();
        MType *int_field = create_type<int>();
        MType *char_field = create_type<char *>();
        marray_char_field_types.push_back(char_field);
        marray_char_field_types.push_back(int_field);
        marray_char_field_types.push_back(int_field);
        marray_user_field_types.push_back(user_type);
        marray_user_field_types.push_back(int_field);
        marray_user_field_types.push_back(int_field);
        element_field_types.push_back(create_struct_reference_type(marray_char_field_types));
        element_field_types.push_back(create_struct_reference_type(marray_user_field_types));
        MPointerType *ptr = create_struct_reference_type(element_field_types);
        return ptr;
    }
};

template <>
struct create_type<File *> {
    operator MPointerType*() {
        std::vector<MType *> marray_char_field_types;
        MType *int_field = create_type<int>();
        MType *char_field = create_type<char *>();
        marray_char_field_types.push_back(char_field);
        marray_char_field_types.push_back(int_field);
        marray_char_field_types.push_back(int_field);
        MPointerType *ptr = create_struct_reference_type(marray_char_field_types);
        return ptr;
    }
};


#endif //MATCHIT_COMPOSITETYPES_H
