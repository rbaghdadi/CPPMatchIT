//
// Created by Jessica Ray on 2/1/16.
//

#include "./Utils.h"

extern "C" void print_sep() {
    fprintf(stderr, "===========================================\n");
}

extern "C" void print_int(int x) {
    fprintf(stderr, "%d\n", x);
}

extern "C" void *malloc_32(size_t size) {
    return malloc(size);
}

extern "C" void *malloc_64(size_t size) {
    return malloc(size);
}

extern "C" void *realloc_32(void *structure, size_t size) {
    return realloc(structure, size);
}

extern "C" void *realloc_64(void *structure, size_t size) {
    return realloc(structure, size);
}