//
// Created by Jessica Ray on 2/1/16.
//

#ifndef MATCHIT_UTILS_H
#define MATCHIT_UTILS_H

#include <vector>
#include <string>
#include "./JIT.h"

/*
 * Various utilities for use in MatchIT IR or llvm IR.
 */

/*
 * Vector functions
 */

/**
 * Return the last extern_input_arg_alloc of a vector
 */
template <typename T>
T last(std::vector<T> vec) {
    return vec[vec.size() - 1];
}

/**
 * Convert a vector to an array
 */
template <typename T>
T* as_array(std::vector<T> *vec) {
    return &((*vec)[0]);
}

/*
 * Wrappers for c functions that can be called in llvm
 */

/**
 * Print a line of '='
 */
extern "C" void print_sep();

/**
 * Print an int using fprintf(stderr,...)
 */
extern "C" void c_fprintf(int i);

/**
 * Print a float using fprintf(stderr,...)
 */
extern "C" void c_fprintf_float(float f);

/**
 * Call C malloc with an i32 llvm type
 */
extern "C" void *malloc_32(size_t size);


/**
 * Call C malloc with an i64 llvm type
 */
extern "C" void *malloc_64(size_t size);

/**
 * Call C realloc with an i32 llvm type
 */
extern "C" void *realloc_32(void *structure, size_t size);

/**
 * Call C realloc with an i64 llvm type
 */
extern "C" void *realloc_64(void *structure, size_t size);


/**
 * Register the functions into llvm that are underlying the above wrappers.
 */
void register_utils(JIT *jit);

#endif //MATCHIT_UTILS_H
