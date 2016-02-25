

#ifndef MATCHIT_STRUCTURES_H
#define MATCHIT_STRUCTURES_H

#include <iostream>

/*
 * Element
 */

template <typename T>
class Element {
private:

    long tag;
    long data_length;
    T *data;

public:

    Element() {
        data_length = 0;
        data = nullptr;
    }

    long get_tag() const {
        return tag;
    }

    long get_data_length() const {
        return data_length;
    }

    T *get_data() const {
        return data;
    }

    void set_tag(long tag) {
        this->tag = tag;
    }

    void set_data(T *data, long data_length) {
        if (!this->data) {
            std::cerr << "About to malloc space for the data array in Element. Make sure this is on purpose!" << std::endl;
            this->data = (T*)malloc(sizeof(T) * data_length);
        }
        memcpy(this->data, data, data_length * sizeof(T));
        this->data_length = data_length;
    }

};

typedef Element<float> FloatElement;

template <typename T>
class Segment {
private:

    long tag;
    long data_length;
    long offset;
    T *data;

public:

    Segment() {
        data_length = 0;
        data = nullptr;
    }

    long get_tag() const {
        return tag;
    }

    long get_data_length() const {
        return data_length;
    }

    T *get_data() const {
        return data;
    }

    void set_tag(long tag) {
        this->tag = tag;
    }

    // used for adding padding
    void add_padding(T *padding, int pad_size) {
        this->data = (T*)realloc(this->data, sizeof(T) * (data_length + pad_size));
        memcpy(&(this->data[data_length]), padding, pad_size * sizeof(T));
    }

    void set_data(T *data, long data_length, long offset) {
        if (!this->data) {
            std::cerr << "About to malloc space for the data array in Element2. Make sure this is on purpose!" << std::endl;
            this->data = (T*)malloc(sizeof(T) * data_length);
        }
        memcpy(this->data, data, data_length * sizeof(T));
        this->data_length = data_length;
        this->offset = offset;
    }

};

typedef Segment<float> FloatSegment;


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
//    ComparisonElement() : BaseElement() { }
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
//    void set_filepath1(MArray<char> *filepath) {
//        this->filepath1 = filepath;
//    }
//
//    void set_filepath2(MArray<char> *filepath) {
//        this->filepath2 = filepath;
//    }
//
//    void set_comparison(T *comparison, size_t num_elements) {
//        this->comparison->add(comparison, num_elements);
//    }
//
//    void set_comparison(MArray<T> *comparison) {
//        this->comparison = comparison;
//    }
//
//
//    /**
//     * Malloc space for T* in data
//     */
//    void malloc_comparison(size_t num_elements) {
//        comparison->mmalloc(num_elements);
//    }
//
//};

#endif //MATCHIT_STRUCTURES_H