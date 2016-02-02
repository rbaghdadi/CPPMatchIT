//
// Created by Jessica Ray on 2/1/16.
//

#ifndef MATCHIT_UTILS_H
#define MATCHIT_UTILS_H

#include <vector>

template <typename T>
T last(std::vector<T> vec) {
    return vec[vec.size() - 1];
}

extern "C" void print_sep();

#endif //MATCHIT_UTILS_H
