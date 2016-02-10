#include <iostream>

#ifndef MATCHIT_COMPOSITETYPES_H
#define MATCHIT_COMPOSITETYPES_H

//class BaseElement : MType {
//protected:
//
//    MArray<char> *filepath;
//
//public:
//
//    BaseElement() {
//        MType *c = create_type<MArray<char> *>();
//        underlying_types.push_back(c);
//    }
//
//    std::vector<MType *> get_underlying_types();
//
//    MArray<char> *get_filepath() {
//        return filepath;
//    }
//
//    void set_filepath(char *filepath, size_t num_chars) {
//        this->filepath->add(filepath, num_chars);
//    }
//
//    void dump() {
//
//    }
//
//    llvm::Type *codegen() {
//
//    }
//
//};

class BaseElement {

};

//template <typename T>
//class WrapperOutput : public BaseElement {
//private:
//
//    MArray<T> **output;
//
//    unsigned int output_length;
//
//public:
//
//    WrapperOutput() : BaseElement() {
//        MType *t = create_type<MArray<T> **>();
//        MType *i = create_type<unsigned int>();
//        underlying_types.push_back(t);
//        underlying_types.push_back(i);
//    }
//
//    MArray<T> **get_output() {
//        return output;
//    }
//
//    unsigned int get_output_length() {
//        return output_length;
//    }
//
//    MArray<char> *get_filepath() {
//        std::cerr << "get_filepath() method not available for Segments." << std::endl;
//        exit(19);
//    }
//
//    MArray<char> set_filepath(char *filepath, size_t num_chars) {
//        std::cerr << "set_filepath() method not available for Segments." << std::endl;
//        exit(19);
//    }
//};

/*
 * MArray
 */

/**
 * An MArray holds an array with a user specified type, along with its size and current end extern_input_arg_alloc idx.
 * The type T of the array must be an MPrimType when specified by the user
 */
template <typename T>
class MArray : public BaseElement {
private:

    T *data; // 8 bytes
    int malloc_size; // 4 bytes
    int cur_idx; // 4 bytes

public:

    MArray() : data(nullptr), malloc_size(0), cur_idx(0) { }

    MArray(int num_elements) : data(nullptr), cur_idx(0) {
        mmalloc(num_elements);
    }

    MArray(T *data, int num_elements) : data(data), cur_idx(0) {
        mmalloc(num_elements);
        add(data, num_elements);
    }

    /**
     * Call C malloc on T* in this MArray.
     */
    void mmalloc(int num_elements) {
        assert(data);
        this->malloc_size = num_elements;
        data = (T*)malloc(sizeof(T) * num_elements);
    }

    /**
     * Call C free on T* in this MArray.
     */
    void mfree() {
        if (data) {
            free(data);
            malloc_size = 0;
            cur_idx = 0;
            data = nullptr;
        }
    }

    /**
     * Call C realloc on T* in this MArray.
     */
    void mrealloc(int new_size) {
        assert(data);
        data = (T*)realloc(data, new_size);
        malloc_size = new_size;
    }

    /**
     * Add a single extern_input_arg_alloc to the end of this MArray.
     */
    void add(T element) {
        if (!data) {
            mmalloc(1);
        } else if (cur_idx + 1 > malloc_size) {
            mrealloc(cur_idx + 1);
        }
        data[cur_idx++] = element;
    }

    // TODO use a memcpy instead
    /**
     * Add an array of extern_input_arg_alloc to the end of this MArray, reallocing
     * if necessary.
     */
    void add(const T* elements, int num_elements) {
        if (!data) {
            mmalloc(num_elements);
        } else if (cur_idx + num_elements > malloc_size) {
            mrealloc(cur_idx + num_elements);
        }
        for (int i = 0; i < num_elements; i++) {
            data[cur_idx++] = elements[i];
        }
    }

    /**
     * Get the current number of elements in data that we have malloced space for.
     */
    int get_malloc_size() {
        return malloc_size;
    }

    /**
     * Get the current index to insert into (i.e. the number of elements currently stored in data).
     */
    int get_cur_idx() {
        return cur_idx;
    }

    /**
     * Get the data array.
     */
    T *get_array() {
        return data;
    }

};

/*
 * File
 */

class File : public BaseElement {
private:

    MArray<char> *filepath;

public:

    File() : BaseElement() { }

    MArray<char> *get_filepath() {
        return filepath;
    };

    void set_filepath(MArray<char> *filepath) {
        this->filepath = filepath;
    }

};

/*
 * Element
 */

template <typename T>
class Element : public BaseElement {
private:

    MArray<char> *filepath;
    MArray<T> *data;

public:

    Element() : BaseElement() { }

    MArray<char> *get_filepath() {
        return filepath;
    }

    MArray<T> *get_data() {
        return data;
    }

    void set_filepath(MArray<char> *filepath) {
        this->filepath = filepath;
    }

    void set_data(T *data, size_t num_elements) {
        this->data->add(data, num_elements);
    }

    /**
     * Malloc space for T* in data
     */
    void malloc_data(size_t num_elements) {
        data->mmalloc(num_elements);
    }

};

/*
 * WrapperOutput
 */

template <typename T>
class WrapperOutput : public BaseElement {
private:

    MArray<T *> *output;

public:

    WrapperOutput() : BaseElement() { }

    MArray<T *> *get_output() {
        return output;
    }

    void add_output_element(T *element) {
        output->add(element);
    }

    /**
   * Malloc space for T* in data
   */
    void malloc_data(size_t num_elements) {
        output->mmalloc(num_elements);
    }

};

//template <typename T>
//class SegmentedElement : public BaseElement {
//private:
//
//    MArray<T> *data;
//    unsigned int offset;
//
//public:
//
//    SegmentedElement() : BaseElement() {
//        MType *t = create_type<MArray<T> *>();
//        MType *i = create_type<unsigned int>();
//        underlying_types.push_back(t);
//        underlying_types.push_back(i);
//    }
//
//    MArray<T> *get_data() {
//        return data;
//    }
//
//    unsigned int get_offset() {
//        return offset;
//    }
//
//    void set_offset(unsigned int offset) {
//        this->offset = offset;
//    }
//
//    void set_data(T *data, size_t num_elements) {
//        this->data->add(data, num_elements);
//    }
//
//    /**
//    * Malloc space for T* in data
//    */
//    void malloc_data(size_t num_elements) {
//        data->mmalloc(num_elements);
//    }
//
//};
//
//template <typename T>
//class Segments : public BaseElement {
//private:
//
//    MArray<SegmentedElement<T> *> *segments;
//
//public:
//
//    Segments() {
//        MType *s = create_type<MArray<SegmentedElement<T> *> *>();
//        underlying_types.push_back(s);
//    }
//
//    MArray<SegmentedElement<T> *> *get_segments() {
//        return segments;
//    }
//
//    void add_segment(SegmentedElement<T> *segment) {
//        segments->add(segment);
//    }
//
//    MArray<char> *get_filepath() {
//        std::cerr << "get_filepath() method not available for Segments." << std::endl;
//        exit(19);
//    }
//
//    MArray<char> set_filepath(char *filepath, size_t num_chars) {
//        std::cerr << "set_filepath() method not available for Segments." << std::endl;
//        exit(19);
//    }
//
//};
//
//template <typename T>
//class ComparisonElement : public BaseElement {
//private:
//
//    MArray<char> *filepath1;
//    MArray<char> *filepath2;
//    MArray<T> *comparison;
//
//public:
//
//    ComparisonElement() {
//        MType *c = create_type<MArray<char> *>();
//        MType *t = create_type<MArray<T> *>();
//        underlying_types.push_back(c);
//        underlying_types.push_back(c);
//        underlying_types.push_back(t);
//    }
//
//    MArray<char> *get_filepath1() {
//        return filepath1;
//    }
//
//    MArray<char> *get_filepath2() {
//        return filepath2;
//    }
//
//    MArray<T> *get_comparison() {
//        return comparison;
//    }
//
//    MArray<char> *get_filepath() {
//        std::cerr << "Need to call get_filepath1() or get_filepath2() in ComparisonElement!" << std::endl;
//        exit(19);
//    }
//
//    MArray<char> set_filepath(char *filepath, size_t num_chars) {
//        std::cerr << "Need to call set_filepath1() or set_filepath2() in ComparisonElement!" << std::endl;
//        exit(19);
//    }
//
//    void set_filepath1(char *filepath, size_t num_chars) {
//        this->filepath1->add(filepath, num_chars);
//    }
//
//    void set_filepath2(char *filepath, size_t num_chars) {
//        this->filepath2->add(filepath, num_chars);
//    }
//
//    /**
//   * Malloc space for T* in data
//   */
//    void malloc_comparison(size_t num_elements) {
//        comparison->mmalloc(num_elements);
//    }
//
//};

#endif //MATCHIT_COMPOSITETYPES_H