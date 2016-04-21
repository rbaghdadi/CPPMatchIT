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

const int fft_size = 65536;
const int seg_size = 75000;
const float overlap = 0.00;
const int dim = 45539;

/**
 * FilterStage. Remove all files that end in txt
 */
Field<char, 200> filter_filepath;
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
 * TransformationStage. Reads in audio.
 */
Field<char, 200> audio_filepath;
Field<float, 1500000> audio;
Field<int> audio_length; // This is needed since we don't have variable size arrays. It's the actual length of the audio
extern "C" void read_audio(const Element * const in, Element * const out) {
    char *fp = in->get(&filter_filepath);
    std::cerr << "reading in " << fp << " with length : ";
    FILE *file = fopen(fp, "rb");
    fseek(file, 0L, SEEK_END);
    int length = ftell(file) / sizeof(float);
    std::cerr << length << std::endl;
    fseek(file, 0L, SEEK_SET);
    float *data = (float*)malloc_32(sizeof(float) * length);
    fread(data, sizeof(float), length, file);
    fclose(file);
    out->set(&audio_filepath, fp);
    out->set(&audio, data);
    out->set(&audio_length, length);
    free(data);
}

/**
 * SegmentationStage. Segment the audio into fixed size chunks.
 */
Field<char, 200> segment_filepath;
Field<float, seg_size> segment_audio;
Field<int> segment_offset;
extern "C" unsigned int segment(const Element * const audio_in, Element ** const segmented_audio) {
    int seg_ctr = 0;
    std::cerr << "segmenting file: " << audio_in->get(&audio_filepath);
    int length = audio_in->get(&audio_length);
    int num_segments = ceil((length - (seg_size*overlap)) / (seg_size - seg_size * overlap));
    std::cerr << " into " << num_segments << " segments." << std::endl;
    float *audio_floats = audio_in->get(&audio);
    if (num_segments <= 0) { // too little data for even a single segment, so just pad it outwards
        num_segments = 1;
    }
    for (int i = 0; i < num_segments; i++) {
        // segsize = 24, overlap = 75%
        // start = 0, start = 6 (1*24*0.25), start = 12, start = 18 (3*24*0.25)
        int start = i * seg_size * (1 - overlap);
        if ((start + seg_size) > length) { // need to pad outwards
            float padded[seg_size];
            int pad_amt = (start + seg_size) - length;
            // get the data that doesn't need padding
            int last_idx = 0;
            for (int j = start; j < length; j++) {
                padded[j - start] = audio_in->get(&audio, j);
                last_idx = j;
            }
            // get the padding
            int next_start = length;
            int ctr = 0;
            for (int j = last_idx; j < last_idx + pad_amt; j++) {
                padded[ctr++] = audio_in->get(&audio, next_start - 1);
            }
            segmented_audio[seg_ctr]->set(&segment_audio, padded);
            segmented_audio[seg_ctr]->set(&segment_offset, start);
            segmented_audio[seg_ctr]->set(&segment_filepath, audio_in->get(&audio_filepath));
        } else {
            segmented_audio[seg_ctr]->set(&segment_audio, &(audio_floats[start]));
            segmented_audio[seg_ctr]->set(&segment_offset, start);
            segmented_audio[seg_ctr]->set(&segment_filepath, audio_in->get(&audio_filepath));
        }
        seg_ctr++;
    }
    return seg_ctr;
}

/**
 * TransformStage. Compute the FFT of the chunks.
 */
Field<char, 200> fft_filepath;
Field<float, fft_size> fft;
Field<int> fft_offset;
extern "C" void compute_fft(const Element * const input, Element * const output) {
    fftwf_plan plan = fftwf_plan_r2r_1d(fft_size, input->get(&segment_audio), output->get(&fft), FFTW_R2HC, FFTW_ESTIMATE);
    // TODO is there a way to make these reusable in my setup? You save some overhead if you don't have to recreate them all the time
    // same with the one in correlation
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
    output->set(&fft_filepath, input->get(&segment_filepath));
    output->set(&fft_offset, input->get(&segment_offset));
    std::cerr << "computing FFT for " << output->get(&fft_filepath) << " that starts at offset " << output->get(&fft_offset) << std::endl;
}

/**
 * ComparisonStage. Correlate the FFTs by multiplying and then applying the inverse FFT.
 */
Field<char, 200> ifft_filepath1;
Field<char, 200> ifft_filepath2;
Field<float, dim> clipped;
// http://dsp.stackexchange.com/questions/736/how-do-i-implement-cross-correlation-to-prove-two-audio-files-are-similar
// In Doug's original code, the pipeline reverses the referent, takes the FFT of that, and then does this computation with
// FFTs of the signal (not reversed). I can't really do the reversal here because I would need to do that in the compute_fft stage,
// and then keep track of which signals are reversed and such so that I don't compare 2 reversed signals. But,
// the link above shows that I can do the reversal after taking the FFT by taking the complex conjugate. This means I can just do that
// to one of the signals in the compute_ifft function. I don't think it matters which one, so just pick fft1.
// TODO I'm not actually doing this yet
extern "C" bool compute_ifft(const Element * const fft1, const Element * const fft2, Element * const ifft_out) {
    if (strcmp(fft1->get(&fft_filepath), fft2->get(&fft_filepath)) == 0) { // only compare if these aren't from the same file
        return false;
    }
    std::cerr << "correlating " << fft1->get(&fft_filepath) << " with offset " << fft1->get(&fft_offset) << " and " << fft2->get(&fft_filepath) << " with offset " << fft2->get(&fft_offset) << std::endl;
    ifft_out->set(&ifft_filepath1, fft1->get(&fft_filepath));
    ifft_out->set(&ifft_filepath2, fft2->get(&fft_filepath));

    // complex multiplication
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

    // compute inverse FFT and threshold the compared values
    unsigned int idx = 0;
    fftwf_plan ifft_plan = fftwf_plan_r2r_1d(fft_size, buff, ifft_buff, FFTW_HC2R, FFTW_ESTIMATE);
    fftwf_execute(ifft_plan);
    for (size_t i = seg_size - 2; i < fft_size; i++) { // MYSTERY
        float clipped_data = ifft_buff[i] / (float)fft_size * 32767.0;
        if (clipped_data > 32767.0) {
            clipped_data = 32767.0;
        } else if (clipped_data < -32767.0) {
            clipped_data = -32767.0;
        }
        ifft_out->set(&clipped, idx++, clipped_data);
    }
    fftwf_destroy_plan(ifft_plan);
    // use all of these comparisons as outputs
    return true;
}

// for reduction
extern "C" int map(const Element * const ifft_result) {
    char *fp1 = ifft_result->get(&ifft_filepath1);
    char *fp2 = ifft_result->get(&ifft_filepath2);
    int cmp = strcmp(fp1, fp2);
    if (cmp < 0) { // fp1 before fp2 in alphabetical order
        // figure out where the null termination character is
        bool found = false;
        int idx = 0;
        while (!found) {
            if (fp1[idx] == '\0') {
                found = true;
            } else {
                idx++;
            }
        }
        char *concat = (char*)malloc_32(sizeof(char)*400);
        strcpy(concat, fp1);
        strcpy(&concat[idx], fp2);
        return std::hash<std::string>()(std::string(concat));
    } else if (cmp > 0) {
        // figure out where the null termination character is
        bool found = false;
        int idx = 0;
        while (!found) {
            if (fp2[idx] == '\0') {
                found = true;
            } else {
                idx++;
            }
        }
        char *concat = (char*)malloc_32(sizeof(char)*400);
        strcpy(concat, fp2);
        strcpy(&concat[idx], fp1);
        return std::hash<std::string>()(std::string(concat));
    } else {
        assert(false); // shouldn't happen since objects weren't compared if they had the same filepath
    }
}

// reduction
extern "C" void recombine(const Element ** const ifft_results) {
    // TODO I don't know how this is supposed to work
}

/**
 * Create input Elements with filepaths.
 */
std::vector<Element *> init_filepaths() {
    std::string fp1 = "/Users/JRay/Desktop/scratch/addrOf.txt";
    std::string fp2 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/test2x2.wav.sw";
    std::string fp3 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/referent.wav.sw";
    std::vector<Element *> elements;
    Element *e1 = new Element(0);
    Element *e2 = new Element(1);
    Element *e3 = new Element(2);
    e1->allocate_and_set(&filter_filepath, &fp1[0]);
    e2->allocate_and_set(&filter_filepath, &fp2[0]);
    e3->allocate_and_set(&filter_filepath, &fp3[0]);
    elements.push_back(e1);
    elements.push_back(e2);
    elements.push_back(e3);
    return elements;
}

int main() {

    JIT *jit = init();

    Fields filter_fields;
    filter_fields.add(&filter_filepath);

    Fields read_audio_fields;
    read_audio_fields.add(&audio_filepath);
    read_audio_fields.add(&audio);
    read_audio_fields.add(&audio_length);

    Fields segment_fields;
    segment_fields.add(&segment_filepath);
    segment_fields.add(&segment_audio);
    segment_fields.add(&segment_offset);

    Fields fft_fields;
    fft_fields.add(&fft_filepath);
    fft_fields.add(&fft);
    fft_fields.add(&fft_offset);

    Fields ifft_fields;
    ifft_fields.add(&ifft_filepath1);
    ifft_fields.add(&ifft_filepath2);
    ifft_fields.add(&clipped);

    std::vector<Element *> elements = init_filepaths();

    FilterStage filt = create_filter_stage(jit, filter, "filter", &filter_fields);
    TransformStage reader = create_transform_stage(jit, read_audio, "read_audio", &filter_fields, &read_audio_fields);
    SegmentationStage segmentor = create_segmentation_stage(jit, segment, "segment", &read_audio_fields, &segment_fields,
                                                            &audio, seg_size, overlap);
    TransformStage fft_xform = create_transform_stage(jit, compute_fft, "compute_fft", &segment_fields, &fft_fields);
    ComparisonStage ifft_xform = create_comparison_stage(jit, compute_ifft, "compute_ifft", &fft_fields, &ifft_fields);

    Pipeline pipeline;
    pipeline.register_stage(&filt);
    pipeline.register_stage(&reader);
    pipeline.register_stage(&segmentor);
    pipeline.register_stage(&fft_xform);
    pipeline.register_stage(&ifft_xform);
    pipeline.codegen(jit);
    jit->dump();
    jit->add_module();

    for (int i = 0; i < 20; i++) {
        run(jit, elements, &audio_filepath, &audio, &audio_length, &segment_filepath, &segment_audio, &segment_offset,
            &fft_filepath, &fft, &fft_offset, &ifft_filepath1, &ifft_filepath2, &clipped);
    }

    return 0;
}
