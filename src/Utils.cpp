//
// Created by Jessica Ray on 2/1/16.
//

#include "./Utils.h"

extern "C" void print_sep() {
    fprintf(stderr, "===========================================\n");
}

extern "C" void print_int(int l) {
    fprintf(stderr, "%d\n", l);
}

extern "C" void print_float(float f) {
    fprintf(stderr, "%f\n", f);
}

//extern "C" void print_string(std::string s) {
//    fprintf(stderr, "%s\n", s);
//}

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