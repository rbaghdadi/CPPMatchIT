//
// Created by Jessica Ray on 4/20/16.
//

#include <stdlib.h>
#include <vector>
#include <iostream>
#include <openssl/md5.h>
#include <llvm/IR/Type.h>
#include <fftw3.h>
#include "../../src/ComparisonStage.h"
#include "../../src/Init.h"
#include "../../src/JIT.h"
#include "../../src/Field.h"
#include "../../src/Pipeline.h"
#include "../../src/StageFactory.h"
#include "../../src/TransformStage.h"
#include "../../src/Utils.h"

const unsigned int fft_size = 65536;
const unsigned int seg_size = 20000;
const float overlap = 0.00;
const int dim = 45539;

// Initial file->audio inputs
Field<char, 200> filepath;
// Filter inputs
Field<char, 200> filter_filepath;
Field<float, 25000> audio; // 2000000
Field<int> audio_length; // This is needed since we don't have variable size arrays. It's the actual length of the audio
// segmentation
Field<char, 200> segment_filepath;
Field<float, seg_size> audio_segment;
Field<int> segment_offset;
// transformation
Field<char, 200> fft_filepath;
Field<float, fft_size> fft;
// comparison
Field<char, 200> ifft_filepath;
Field<float, dim> clipped;

/**
 * TransformationStage. Reads in audio.
 */
extern "C" void read_audio(const Element * const in, Element * const out) {
    char *fp = in->get(&filepath);
    std::cerr << "reading in " << fp << " with length : ";
    FILE *file = fopen(fp, "rb");
    fseek(file, 0L, SEEK_END);
    int length = ftell(file) / sizeof(float);
    std::cerr << length << std::endl;
    fseek(file, 0L, SEEK_SET);
    float *data = (float*)malloc_32(sizeof(float) * length);
    fread(data, sizeof(float), length, file);
    fclose(file);
    out->set(&filter_filepath, fp);
    out->set(&audio, data);
    out->set(&audio_length, length);
    free(data);
}

/**
 * FilterStage. Remove all files that end in txt
 */
extern "C" bool filter(const Element * const input) {
    char *file = input->get(&filter_filepath);
    bool keep = strstr(file, "txt") == nullptr; // if == nullptr, keep it because it does NOT end in txt (can't FFT a txt file...)
    if (keep) {
        std::cerr << "Keeping file: " << file << std::endl;
    } else {
        std::cerr << "Dropping file: " << file << std::endl;
    }
    return keep;
}

/**
 * SegmentationStage. Segment the audio into fixed size chunks.
 */
extern "C" unsigned int segment(const Element * const audio_in, Element ** const segmented_audio) {
    int seg_ctr = 0;
    std::cerr << "segmenting file: " << audio_in->get(&filter_filepath) << std::endl;
//    int length = audio_in->get(&audio_length);
//    int num_segments = ceil((length - (seg_size*overlap)) / (seg_size - seg_size * overlap));
//    float *audio_floats = audio_in->get(&audio);
//    if (num_segments <= 0) { // too little data for even a single segment, so just pad it outwards
//        num_segments = 1;
//    }
//    for (int i = 0; i < num_segments; i++) {
//        int start = i * seg_size * overlap;
//        if ((start + seg_size) > length) { // need to pad outwards
//            float padded[seg_size];
//            int pad_amt = (start + seg_size) - length;
//            // get the data that doesn't need padding
//            int last_idx = 0;
//            for (int j = start; j < length; j++) {
//                padded[j - start] = audio_in->get(&audio, j);
//                last_idx = j;
//            }
//            // get the padding
//            int next_start = length;
//            for (int j = last_idx; j < last_idx + pad_amt; j++) {
//                padded[j] = audio_in->get(&audio, next_start - 1);
//            }
//            segmented_audio[seg_ctr]->set(&audio_segment, padded);
//            segmented_audio[seg_ctr]->set(&segment_offset, start);
//        } else {
//            segmented_audio[seg_ctr]->set(&audio_segment, &(audio_floats[start]));
//            segmented_audio[seg_ctr]->set(&segment_offset, start);
//        }
//        seg_ctr++;
//    }
    return seg_ctr;
}

/**
 * TransformStage. Compute the FFT of the chunks.
 */
extern "C" void compute_fft(const Element * const input, Element * const output) {
    fftwf_plan plan = fftwf_plan_r2r_1d(fft_size, input->get(&audio), output->get(&fft), FFTW_R2HC, FFTW_ESTIMATE);
    // TODO is there a way to make these reusable in my setup? You save some overhead if you don't have to recreate them all the time
    // same with the one in correlation
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
}

/**
 * ComparisonStage. Correlate the FFTs by multiplying and then applying the inverse FFT.
 */
extern "C" bool correlate(const Element * const fft1, const Element * const fft2, Element * const correlation) {
    float *fft1_fft = fft1->get(&fft);
    float *fft2_fft = fft2->get(&fft);
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

    fftwf_plan ifft_plan = fftwf_plan_r2r_1d(fft_size, buff, ifft_buff, FFTW_HC2R, FFTW_ESTIMATE);

    unsigned int idx = 0;
    // compute inverse FFT and threshold the compared values
    fftwf_execute(ifft_plan);
    for (size_t i = seg_size - 2; i < fft_size; i++) { // MYSTERY
        //  for (size_t i = 0; i < fft_size; i++) {
        float clipped_data = ifft_buff[i] / (float)fft_size * 32767.0;
        if (clipped_data > 32767.0) {
            clipped_data = 32767.0;
        } else if (clipped_data < -32767.0) {
            clipped_data = -32767.0;
        }
        correlation->set(&clipped, idx++, clipped_data);
    }
    fftwf_destroy_plan(ifft_plan);
    // use all of these comparisons as outputs
    return true;
}

/**
 * Create input Elements with filepaths.
 */
std::vector<Element *> init_filepaths() {
    std::string fp1 = "/Users/JRay/Desktop/scratch/addrOf.txt";
    std::string fp2 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/referent.wav.sw";
    std::string fp3 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/referent.wav.sw";
    std::vector<Element *> elements;
    Element *e1 = new Element(0);
    Element *e2 = new Element(1);
    Element *e3 = new Element(2);
    e1->allocate_and_set(&filepath, &fp1[0]);
    e2->allocate_and_set(&filepath, &fp2[0]);
    e3->allocate_and_set(&filepath, &fp3[0]);
    elements.push_back(e1);
    elements.push_back(e2);
    elements.push_back(e3);
    return elements;
}

int main() {

    JIT *jit = init();

    Fields input_fields;
    input_fields.add(&filepath);

    Fields filter_fields;
    filter_fields.add(&filter_filepath);
    filter_fields.add(&audio);
    filter_fields.add(&audio_length);

    Fields segment_fields;
    segment_fields.add(&segment_filepath);
    segment_fields.add(&audio_segment);
    segment_fields.add(&segment_offset);

    Fields fft_fields;
    fft_fields.add(&fft_filepath);
    fft_fields.add(&fft);

    Fields ifft_fields;
    ifft_fields.add(&ifft_filepath);
    ifft_fields.add(&clipped);

    std::vector<Element *> elements = init_filepaths();

    FilterStage filt = create_filter_stage(jit, filter, "filter", &filter_fields); // TODO reorder these fields
    TransformStage reader = create_transform_stage(jit, read_audio, "read_audio", &input_fields, &filter_fields);
    SegmentationStage segmentor = create_segmentation_stage(jit, segment, "segment", &filter_fields, &segment_fields,
                                                            &audio, seg_size, overlap);
    TransformStage fft_xform = create_transform_stage(jit, compute_fft, "compute_fft", &segment_fields, &fft_fields);
    ComparisonStage ifft_xform = create_comparison_stage(jit, correlate, "correlate", &fft_fields, &ifft_fields);

    Pipeline pipeline;
    pipeline.register_stage(&reader);
    pipeline.register_stage(&filt);
    pipeline.register_stage(&segmentor);
//    pipeline.register_stage(&fft_xform);
//    pipeline.register_stage(&ifft_xform);
    pipeline.codegen(jit);
    jit->dump();
    jit->add_module();

    run(jit, elements, &filter_filepath, &audio, &audio_length, &segment_filepath, &audio_segment, &segment_offset,
        &fft_filepath, &fft, &ifft_filepath, &clipped);

    return 0;
}
