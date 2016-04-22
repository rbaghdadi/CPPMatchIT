//
// Created by Jessica Ray on 4/22/16.
//

#ifndef MATCHIT_FILEDUPS_H
#define MATCHIT_FILEDUPS_H

#include <openssl/md5.h>
#include "../../src/Field.h"

namespace FileDups {

/*
 * FilterStage function
 */
// if returns true, keep the element
extern Field<char, 200> filepath_in; // holds the filepath (an input)
extern "C" bool filter_file(const Element *const in);

/*
 * TransformStage fields and function
 */
extern Field<char, 200> filepath_out; // if field is not going to change, should it just be pointed to the same place?
extern Field<unsigned char, MD5_DIGEST_LENGTH> md5_out; // holds the computed md5 (an output)
extern "C" void file_to_md5(const Element *const in, Element *const out);

/*
 * ComparisonStage function
 */
extern "C" bool compare(const Element *const in1, const Element *const in2);

void setup(JIT *jit);

void start(JIT *jit);

}

#endif //MATCHIT_FILEDUPS_H
