#include <stdlib.h>
#include <vector>
#include <iostream>
#include <openssl/md5.h>
#include <llvm/IR/Type.h>
#include <fftw3.h>
#include "./LLVM.h"
#include "./JIT.h"
#include "./Utils.h"
#include "./Field.h"
#include "./TransformStage.h"
#include "./StageFactory.h"
#include "./Pipeline.h"

/*
 * File comparison
 */
TemplatedDebug<float> f;
Field<char,200> filepath_field1;
Field<char,200> filepath_field2;
Field<unsigned char,100> md5_field;
Field<float> float_field;

extern "C" void compute_md5(const SetElement * const in, SetElement * const out) {
    char *filepath = in->get(&filepath_field1);
    std::cerr << "computing md5 for: " << filepath << std::endl;
    // read in file
    FILE *file = fopen(filepath, "rb");
    fseek(file, 0L, SEEK_END);
    int length = ftell(file) / sizeof(unsigned char);
    fseek(file, 0L, SEEK_SET);
    unsigned char data[length];
    fread(data, sizeof(unsigned char), length, file);
    fclose(file);
    // compute md5
    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, data, length);
    out->set(&filepath_field2, filepath);
    MD5_Final(out->get(&md5_field), &context);
    char md5_str[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_str[i * 2], "%02x", out->get(&md5_field, i));
    }
    fprintf(stderr, " -> md5 digest: %s for file %s\n", md5_str, out->get(&filepath_field2));
}

bool filter_file(const SetElement * const in) {
    char *filepath = in->get(&filepath_field1);
    return strstr(filepath, "txt") == nullptr; // remove if suffix contains txt
}

bool compare(const SetElement * const in1, const SetElement * const in2) {
    unsigned char *computed_md5_1 = in1->get(&md5_field);
    unsigned char *computed_md5_2 = in2->get(&md5_field);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (computed_md5_1[i] != computed_md5_2[i]) {
            return false;
        }
    }
    return true;
}

const unsigned int window_size = 10;
const unsigned int min_threshold = 5;
const unsigned int sequence_length = 100;

Field<char, 100> sequence_name_field;
Field<char, 100> sequence_name_field1;
Field<char, 100> sequence_name_field2;
Field<char, 100> sequence_type_field;
Field<char, sequence_length> sequence_field;
Field<char, sequence_length> sequence_field1;
Field<char, sequence_length> sequence_field2;
Field<bool, sequence_length, sequence_length> dot_plot_field;

/*
 * Dot plots
 */

bool filter_sequence_types(const SetElement * const sequence) {
    return strcmp(sequence->get(&sequence_type_field), "someLongBiologyWord") == 0;
}

// This is like a reduction function. It just prints stuff so it doesn't need an output
void print_plot(const SetElement * const plot) {
    std::string filename1(plot->get(&sequence_name_field1));
    std::string filename2(plot->get(&sequence_name_field1));
    FILE *output = fopen(("/Users/JRay/Desktop/" + filename1 + filename2).c_str(), "w");
    fprintf(output, "    ");
    for (unsigned int i = 0; i < sequence_length; i++) {
        char the_char = plot->get(&sequence_field1, i);
        fprintf(output, "%c", the_char);
    }
    fprintf(output, "\n");
    for (unsigned int i = 0; i < sequence_length; i++) {
        fprintf(output, "-");
    }
    fprintf(output, "\n");
    for (unsigned int y = 0; y < sequence_length; y++) {
        char the_char = plot->get(&sequence_field2, y);//get_domain_data<char, sequence_length>("bases2", y);
        fprintf(output, "%c |", the_char);
        for (unsigned int x = 0; x < sequence_length; x++) {
            if (plot->get(&dot_plot_field, x, y)) {//get_domain_data<bool, sequence_length, sequence_length>("dotPlot", x, y)) {
                fprintf(output, "x");
            } else {
                fprintf(output, " ");
            }
        }
        fprintf(output, "\n");
    }
    fflush(output);
    fclose(output);
}

bool plot(const SetElement * const sequence1, const SetElement * const sequence2, SetElement *plot) {
    int frame = window_size / 2;
    plot->set(&sequence_name_field1, sequence1->get(&sequence_name_field));
    plot->set(&sequence_name_field2, sequence2->get(&sequence_name_field));
    for (unsigned int x = frame; x < sequence_length; x++) {
        for (unsigned int y = frame; y < sequence_length; y++) {
            uint matches = 0;
            int lower_bound = -frame;
            int upper_bound = frame;
            do {
                char sequence1_char = sequence1->get(&sequence_field, x + lower_bound);
                char sequence2_char = sequence1->get(&sequence_field, y + lower_bound);
                if (sequence1_char == sequence2_char) {
                    matches++;
                }
                lower_bound++;
            } while (lower_bound <= upper_bound);
            plot->set(&dot_plot_field, x, y, matches >= min_threshold);
        }
    }
    return true;
}

/*
 * FFT
 */

const unsigned int fft_size = 65536;
const unsigned int seg_size = 20000;
const float overlap = 0.75;
const int dim = 45539;// fft_size - (seg_size - 2) + 1;

Field<char, 200> fft_file_field;
Field<float, 50000> audio_field;

Field<float, fft_size> fft_field;
Field<float, dim> clipped_field;

void run_fft(const SetElement * const input, SetElement *output) {
    fftwf_plan plan = fftwf_plan_r2r_1d(fft_size,
                                        input->get(&audio_field),
                                        output->get(&fft_field), FFTW_R2HC, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
}

bool correlate(const SetElement * const fft1, const SetElement * const fft2, SetElement * correlation) {
    float *fft1_fft = fft1->get(&fft_field);
    float *fft2_fft = fft2->get(&fft_field);
    float buff[fft_size];
    float ifft_buff[fft_size];
    for (size_t real_idx = 1; real_idx < fft_size / 2; real_idx++) {
        unsigned int imag_idx = fft_size - real_idx;

        float real = fft1_fft[real_idx] * fft2_fft[real_idx] - fft1_fft[imag_idx] * fft2_fft[imag_idx];
        float imag = fft1_fft[real_idx] * fft2_fft[imag_idx] + fft1_fft[imag_idx] * fft2_fft[real_idx];
        buff[real_idx] = real;
        buff[imag_idx] = imag;
    }

    // the imaginary part of these is 0, so it is not stored
    buff[0] = fft1_fft[0] * fft2_fft[0];
    buff[fft_size/2] = fft1_fft[fft_size/2] * fft2_fft[fft_size/2];

    // TODO is there any way to reuse this? I could create a dummy one to begin with, but then you can't run it in parallel (at least I don't think you can)
    fftwf_plan ifft_plan = fftwf_plan_r2r_1d(fft_size, buff, ifft_buff, FFTW_HC2R, FFTW_ESTIMATE);

    unsigned int idx = 0;
    // compute inverse FFT and threshold the compared values
    fftwf_execute(ifft_plan);
    for (size_t i = seg_size - 2; i < fft_size; i++) { // MYSTERY
        //  for (size_t i = 0; i < fft_size; i++) {
        float clipped = ifft_buff[i] / (float)fft_size * 32767.0;
        if (clipped > 32767.0) {
            clipped = 32767.0;
        } else if (clipped < -32767.0) {
            clipped = -32767.0;
        }
        correlation->set(&clipped_field, idx++, clipped);
    }

    return true;
}

int main() {

    LLVM::init();
    JIT jit;
    register_utils(&jit);

    Relation in;
    Relation out;
    in.add(&filepath_field1);
    in.add(&float_field);
    out.add(&filepath_field2);
    out.add(&md5_field);

    // Inputs
    SetElement *e1 = new SetElement();
    SetElement *e2 = new SetElement();
    SetElement *e3 = new SetElement();

    // initialize some data and attach the SetElements
    std::string fname1 = "/Users/JRay/Desktop/scratch/test2.cpp";
    std::string fname2 = "/Users/JRay/Desktop/scratch/conversionTest.cpp";
    e1->init(&filepath_field1, fname1.c_str());
    e1->init(&float_field, 17.0f);
    e2->init(&filepath_field1, fname2.c_str());
    e2->init(&float_field, 29.0f);
    e3->init(&filepath_field1, fname2.c_str());
    e3->init(&float_field, 37.0f);

    // attach output SetElements
    SetElement *e4 = new SetElement();
    SetElement *e5 = new SetElement();
    SetElement *e6 = new SetElement();
    e4->init_no_malloc(&filepath_field2);
    e4->init_no_malloc(&md5_field);
    e5->init_no_malloc(&filepath_field2);
    e5->init_no_malloc(&md5_field);
    e6->init_no_malloc(&filepath_field2);
    e6->init_no_malloc(&md5_field);

//    compute_md5(e1, e4);
//    compute_md5(e2, e5);
//    compute_md5(e3, e6);

//    std::cerr << "running e4,e4" << std::endl;
//    assert(compare(e4, e4));
//    std::cerr << "running e4,e5" << std::endl;
//    assert(!compare(e4, e5));
//    std::cerr << "running e4,e6" << std::endl;
//    assert(!compare(e4, e6));
//    std::cerr << "running e5,e5" << std::endl;
//    assert(compare(e5, e5));
//    std::cerr << "running e5,e6" << std::endl;
//    assert(compare(e5, e6));
//    std::cerr << "running e6,e6" << std::endl;
//    assert(compare(e6, e6));

    TransformStage xform = create_transform_stage(&jit, compute_md5, "compute_md5", &in, &out);
    FilterStage filt = create_filter_stage(&jit, filter_file, "filter_file", &out);

    Pipeline pipeline;
    pipeline.register_stage(&xform, &in, &out);
//    pipeline.register_stage(&filt, &out);
    pipeline.codegen(&jit);
    jit.dump();
    jit.add_module();

    // currently, we will just manually specify the input and output SetElements
    std::vector<SetElement *> in_setelements;
    in_setelements.push_back(e1);
    in_setelements.push_back(e2);
    in_setelements.push_back(e3);
    std::vector<SetElement *> out_setelements;
    out_setelements.push_back(e4);
    out_setelements.push_back(e5);
    out_setelements.push_back(e6);

    std::vector<BaseField *> one;
    one.push_back(&filepath_field2);
    std::vector<BaseField *> two;
    two.push_back(&md5_field);

//    runMacro(jit, in_setelements, out_setelements, &(one[0]), &(two[0]));
    runMacro(jit, in_setelements, out_setelements, &filepath_field2, &md5_field);

    return 0;
}
