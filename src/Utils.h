//
// Created by Jessica Ray on 2/1/16.
//

#ifndef MATCHIT_UTILS_H
#define MATCHIT_UTILS_H

#include <vector>
#include <string>
#include "./JIT.h"

// following split methods are from http://stackoverflow.com/questions/236129/split-a-string-in-c
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);

std::vector<std::string> split(const std::string &s, char delim);

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
