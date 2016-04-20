//
// Created by Jessica Ray on 4/4/16.
//

#include <openssl/md5.h>
#include "../../src/ComparisonStage.h"
#include "../../src/Field.h"
#include "../../src/FilterStage.h"
#include "../../src/Init.h"
#include "../../src/JIT.h"
#include "../../src/LLVM.h"
#include "../../src/Pipeline.h"
#include "../../src/StageFactory.h"
#include "../../src/TransformStage.h"
#include "./Files.h"

/*
 * FilterStage function
 */
// if returns true, keep the element
Field<char, 200> filepath_in; // holds the filepath (an input)
extern "C" bool filter_file(const Element * const in) {
    char *filepath = in->get(&filepath_in);
    bool keep = strstr(filepath, "txt") == nullptr; // if == nullptr, keep it because it does NOT end in txt
    if (keep) {
        std::cerr << "Keeping " << filepath << std::endl;
    } else {
        std::cerr << "Dropping " << filepath << std::endl;
    }
    return keep;
}

/*
 * TransformStage fields and function
 */
Field<char, 200> filepath_out; // if field is not going to change, should it just be pointed to the same place?
Field<unsigned char, MD5_DIGEST_LENGTH> md5_out; // holds the computed md5 (an output)
extern "C" void file_to_md5(const Element * const in, Element * const out) {
    char *filepath = in->get(&filepath_in);
    std::cerr << "computing md5 for: " << filepath << std::endl;
    // read in file
    FILE *file = fopen(filepath, "rb");
    fseek(file, 0L, SEEK_END);
    int length = ftell(file) / sizeof(unsigned char);
    fseek(file, 0L, SEEK_SET);
    unsigned char *data = (unsigned char*)malloc(sizeof(unsigned char) * length);
    fread(data, sizeof(unsigned char), length, file);
    fclose(file);
    // compute md5
    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, data, length);
    out->set(&filepath_out, filepath);
    MD5_Final(out->get(&md5_out), &context); // does this instead of calling set
    free(data);
    char md5_str[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_str[i * 2], "%02x", out->get(&md5_out, i));
    }
    fprintf(stderr, " -> md5 digest: %s for file %s\n", md5_str, out->get(&filepath_out));
}

/*
 * ComparisonStage function
 */
extern "C" bool compare(const Element * const in1, const Element * const in2) {
    unsigned char *computed_md5_1 = in1->get(&md5_out);
    unsigned char *computed_md5_2 = in2->get(&md5_out);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (computed_md5_1[i] != computed_md5_2[i]) {
            std::cerr << in1->get(&filepath_out) << " and " << in2->get(&filepath_out) << " do not match." << std::endl;
            return false;
        }
    }
    std::cerr << in1->get(&filepath_out) << " and " << in2->get(&filepath_out) << " match." << std::endl;
    return true;
}

int main() {

    JIT *jit = init();

    Fields filter_in; // passed into filter_file

    Fields file_to_md5_out; // passed out of file_to_md5
    filter_in.add(&filepath_in);
    file_to_md5_out.add(&filepath_out);
    file_to_md5_out.add(&md5_out);

    std::vector<Element *> in_elements = init(&filepath_in);

    FilterStage filt = create_filter_stage(jit, filter_file, "filter_file", &filter_in);
    TransformStage xform = create_transform_stage(jit, file_to_md5, "file_to_md5", &filter_in, &file_to_md5_out);
    ComparisonStage comp = create_comparison_stage(jit, compare, "compare", &file_to_md5_out);

    Pipeline pipeline;
    pipeline.register_stage(&filt);
    pipeline.register_stage(&xform);
    pipeline.register_stage(&comp);
    pipeline.codegen(jit);
    jit->dump();
    jit->add_module();

    // add the output fields here (add all across the stages)
    run(jit, in_elements, &filepath_out, &md5_out);

    return 0;
}