//
// Created by Jessica Ray on 2/1/16.
//

#ifndef MATCHIT_UTILS_H
#define MATCHIT_UTILS_H

#include <vector>
#include <string>

template <typename T>
T last(std::vector<T> vec) {
    return vec[vec.size() - 1];
}

template <typename T>
T* as_array(std::vector<T> *vec) {
    return &((*vec)[0]);
}

extern "C" void print_sep();

extern "C" void print_int(int x);

extern "C" void *malloc_32(size_t size);

extern "C" void *malloc_64(size_t size);

extern "C" void *realloc_32(void *structure, size_t size);

extern "C" void *realloc_64(void *structure, size_t size);

//
//template <typename T>
//int get_size_in_bytes(T element) {
//    return sizeof(T);
//}
//
//template <>
//int get_size_in_bytes(std::string element) {
//    return sizeof(element) * element.size();
//}
//
//template <>
//int get_size_in_bytes(char *element) { // is this actually legit?
//    int cur_size = 0;
//    char cur_char = element[0];
//    while (cur_char != '\0') {
//        cur_size++;
//        cur_char = element[cur_size];
//    }
//    return cur_size;
//}

#endif //MATCHIT_UTILS_H
