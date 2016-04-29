//
// Created by Jessica Ray on 4/22/16.
//

#include <cfloat>
#include <fftw3.h>
#include "../../src/ComparisonStage.h"
#include "../../src/FilterStage.h"
#include "../../src/Pipeline.h"
#include "../../src/SegmentationStage.h"
#include "../../src/StageFactory.h"
#include "../../src/TransformStage.h"
#include "./MF.h"

namespace MF {

Field<char, 200> filter_filepath;
Field<char, 200> audio_filepath;
Field<float, 2500000> audio;
Field<int> audio_length;
Field<char, 200> segment_filepath;
Field<float, seg_size> segment_audio;
Field<int> segment_offset;
Field<char, 200> fft_filepath;
Field<float, fft_size> fft;
Field<int> fft_offset;
Field<char, 200> ifft_filepath1;
Field<char, 200> ifft_filepath2;
Field<int> ifft_offset1;
Field<int> ifft_offset2;
Field<float, dim> clipped;

extern "C" bool filter(const Element * const input) {
    char *file = input->get(&filter_filepath);
    bool keep = strstr(file, "txt") ==
                nullptr; // if == nullptr, keep it because it does NOT end in txt (can't FFT a txt file...)
    if (keep) {
        std::cerr << "Keeping file: " << file << std::endl;
    } else {
        std::cerr << "Dropping file: " << file << std::endl;
    }
    return keep;
}

extern "C" void read_audio(const Element * const in, Element * const out) {
    char *fp = in->get(&filter_filepath);
    std::cerr << "reading in " << fp << " with length : ";
    FILE *file = fopen(fp, "rb");
    fseek(file, 0L, SEEK_END);
    int length = ftell(file) / sizeof(short);
    int padded_length = (int)(ceil(((float)length)/((float)fft_size)))*fft_size;
    std::cerr << length << " and padded length: " << padded_length << std::endl;
    std::cerr << "fp length: " << strlen(fp) << std::endl;
    // read audio
    fseek(file, 0L, SEEK_SET);
    short *tmp_data = (short*)mmalloc(sizeof(short) * length);
    fread(tmp_data, sizeof(short), length, file);
    fclose(file);

    // scale the audio
    float scale = 0.0;
    for (int i = 0; i < length; i++) {
        scale += tmp_data[i]*tmp_data[i];
    }
    scale = sqrt(scale);
    scale = (scale < 1.0/FLT_MAX) ? 0.0 : 0.5/scale;
    float *data = (float*)mmalloc(sizeof(float) * padded_length);
    for (int i = 0; i < length; i++) {
        data[i] = (float)tmp_data[i] * scale;
    }

    // pad the rest of data with 0s
    for (int i = length; i < padded_length; i++) {
        data[i] = 0.0;
    }

    // +1 in strlen because need null terminator
    out->set(&audio_filepath, fp, strlen(fp)+1);
    out->set(&audio, data, padded_length);
    out->set(&audio_length, padded_length);
    free(tmp_data);
    free(data);
}

extern "C" unsigned int compute_num_segments(const Element * audio_in) {
    int length = audio_in->get(&audio_length);
    return ceil(((float)length - ((float)seg_size * overlap)) / ((float)seg_size - ((float)seg_size * overlap)));
//    return length / (int)((float)seg_size * overlap);
}

extern "C" unsigned int segment(const Element * const audio_in, Element ** const segmented_audio) {
    int seg_ctr = 0;
    std::cerr << "segmenting file: " << audio_in->get(&audio_filepath);
    int length = audio_in->get(&audio_length);
    int num_segments = compute_num_segments(audio_in);
    std::cerr << " into " << num_segments << " segments." << std::endl;
    float *audio_floats = audio_in->get(&audio);
    if (num_segments <= 0) { // too little data for even a single segment, so just pad it outwards
        num_segments = 1;
    }
    // assumes data is correctly padded already
    for (int i = 0; i < length && i + seg_size - 1 <= length; i+=(int)((float)seg_size*overlap)) {
        segmented_audio[seg_ctr]->set(&segment_audio, &(audio_floats[i]), seg_size);
        segmented_audio[seg_ctr]->set(&segment_offset, i);
        segmented_audio[seg_ctr]->set(&segment_filepath, audio_in->get(&audio_filepath), strlen(audio_in->get(&audio_filepath)) + 1);
        seg_ctr++;

    }
    return num_segments;
}

//extern "C" unsigned int segment(const Element * const audio_in, Element ** const segmented_audio) {
//    int seg_ctr = 0;
//    std::cerr << "segmenting file: " << audio_in->get(&audio_filepath);
//    int length = audio_in->get(&audio_length);
//    int num_segments = ceil((length - (seg_size*overlap)) / (seg_size - seg_size * overlap));
//    std::cerr << " into " << num_segments << " segments." << std::endl;
//    float *audio_floats = audio_in->get(&audio);
//    if (num_segments <= 0) { // too little data for even a single segment, so just pad it outwards
//        num_segments = 1;
//    }
//    for (int i = 0; i < num_segments; i++) {
//        // segsize = 24, overlap = 75%
//        // start = 0, start = 6 (1*24*0.25), start = 12, start = 18 (3*24*0.25)
//        int start = i * seg_size * (1 - overlap);
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
//            int ctr = 0;
//            for (int j = last_idx; j < last_idx + pad_amt; j++) {
//                padded[ctr++] = audio_in->get(&audio, next_start - 1);
//            }
//            segmented_audio[seg_ctr]->set(&segment_audio, padded);
//            segmented_audio[seg_ctr]->set(&segment_offset, start);
//            segmented_audio[seg_ctr]->set(&segment_filepath, audio_in->get(&audio_filepath));
//        } else {
//            segmented_audio[seg_ctr]->set(&segment_audio, &(audio_floats[start]));
//            segmented_audio[seg_ctr]->set(&segment_offset, start);
//            segmented_audio[seg_ctr]->set(&segment_filepath, audio_in->get(&audio_filepath));
//        }
//        seg_ctr++;
//    }
//    return seg_ctr;
//}

extern "C" void compute_fft(const Element * const input, Element * const output) {
    output->set(&fft_filepath, input->get(&segment_filepath));
    output->set(&fft_offset, input->get(&segment_offset));
    std::cerr << "computing FFT for " << output->get(&fft_filepath) << " that starts at offset " << output->get(&fft_offset) << std::endl;
    fftwf_plan plan = fftwf_plan_r2r_1d(fft_size, input->get(&segment_audio), output->get(&fft), FFTW_R2HC, FFTW_ESTIMATE);
    // TODO is there a way to make these reusable in my setup? You save some overhead if you don't have to recreate them all the time
    // same with the one in correlation
    fftwf_execute(plan);
    float *x = output->get(&fft);
    fftwf_destroy_plan(plan);
}

extern "C" bool compute_ifft(const Element * const fft1, const Element * const fft2, Element * const ifft_out) {
    if (strcmp(fft1->get(&fft_filepath), fft2->get(&fft_filepath)) == 0) { // only compare if these aren't from the same file
        return false;
    }
    std::cerr << "correlating " << fft1->get(&fft_filepath) << " with offset " << fft1->get(&fft_offset) << " and " << fft2->get(&fft_filepath) << " with offset " << fft2->get(&fft_offset) << std::endl;
    ifft_out->set(&ifft_filepath1, fft1->get(&fft_filepath));
    ifft_out->set(&ifft_filepath2, fft2->get(&fft_filepath));
    ifft_out->set(&ifft_offset1, fft1->get(&fft_offset));
    ifft_out->set(&ifft_offset2, fft2->get(&fft_offset));

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

// assumes incoming elements are grouped correctly (by file) and sorted by offsets
extern "C" void recombine(const Element ** const elements, int num_elements) {
    int output_ctr = 0;
    // store indices across the iFFT arrays if that index corresponds to a value >= threshold
    int *start_idxs = (int*)mmalloc(sizeof(int) * num_elements * (fft_size - seg_size + 2));
    int *end_idxs = (int*)mmalloc(sizeof(int) * num_elements * (fft_size - seg_size + 2));
    // the offsets corresponding to the above indices
    int *offsets1 = (int*)mmalloc(sizeof(int) * num_elements * (fft_size - seg_size + 2));
    int *offsets2 = (int*)mmalloc(sizeof(int) * num_elements * (fft_size - seg_size + 2));

    // go through and find values above the threshold. The corresponding offsets will define the regions to keep
    for (int i = 0; i < num_elements; i++) {
        for (int j = 0; j < fft_size - seg_size + 2; j++) {
            const Element *element = elements[i];
            float val = element->get(&clipped, j);
            if (val >= threshold) {
                start_idxs[output_ctr] = i * (overlap * seg_size) + j;
                end_idxs[output_ctr] = i * (overlap * seg_size) + j + seg_size;
                offsets1[output_ctr] = element->get(&ifft_offset1);
                offsets2[output_ctr] = element->get(&ifft_offset2);
                output_ctr++;
            }
        }
    }

    // merge segments together to get the maximal matching regions
    int *merged_idx = (int*)mmalloc(sizeof(int) * output_ctr); // indices
    int prev_idx = 0;
    int final_idx = 0;
    int *merged_starts1 = (int*)mmalloc(sizeof(int) * output_ctr);
    int *merged_starts2 = (int*)mmalloc(sizeof(int) * output_ctr);
    int *merged_ends1 = (int*)mmalloc(sizeof(int) * output_ctr);
    int *merged_ends2 = (int*)mmalloc(sizeof(int) * output_ctr);
    merged_idx[0] = end_idxs[0];
    for (int i = 0; i < output_ctr; i++) {
        if (merged_idx[final_idx] > start_idxs[i] && i == prev_idx + 1) {
            merged_idx[final_idx] = end_idxs[i];
            merged_ends1[final_idx] = offsets1[i];
            merged_ends2[final_idx] = offsets2[i];
        } else {
            final_idx++;
            merged_idx[final_idx] = end_idxs[i];
            merged_starts1[final_idx] = offsets1[i];
            merged_ends1[final_idx] = offsets1[i];
            merged_starts2[final_idx] = offsets2[i];
            merged_ends2[final_idx] = offsets2[i];
        }
        prev_idx = i;
    }
    final_idx++;

    // print results
    for (int i = 0; i < final_idx; i++) {
        fprintf(stderr, "%s from %.6f to %.6f, %s from %.6f to %.6f\n",
                elements[0]->get(&ifft_filepath1), (float)merged_starts1[i] / sample_rate,
                ((float)merged_ends1[i] + (float)seg_size) / sample_rate, elements[0]->get(&ifft_filepath2),
                (float)merged_starts2[i] / sample_rate, ((float)merged_ends2[i] + (float)seg_size) / sample_rate);
    }

    free(start_idxs);
    free(end_idxs);
    free(offsets1);
    free(offsets2);
    free(merged_idx);
    free(merged_starts1);
    free(merged_starts2);
    free(merged_ends1);
    free(merged_ends2);
}

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
        char *concat = (char*) mmalloc(sizeof(char) * 400);
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
        char *concat = (char*) mmalloc(sizeof(char) * 400);
        strcpy(concat, fp2);
        strcpy(&concat[idx], fp1);
        return std::hash<std::string>()(std::string(concat));
    } else {
        assert(false); // shouldn't happen since objects weren't compared if they had the same filepath
    }
}

std::vector<Element *> init_filepaths() {
    std::string fp1 = "/Users/JRay/Desktop/scratch/addrOf.txt";
    std::string fp2 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/test2x2.wav.sw";
    std::string fp3 = "/Users/JRay/Documents/Research/MatchedFilter/test_sigs/referent.wav.sw";
    std::vector<Element *> elements;
    Element *e1 = new Element(0);
    Element *e2 = new Element(1);
    Element *e3 = new Element(2);
    e1->allocate_and_set(&filter_filepath, &fp1[0], fp1.length() + 1); // +1 b/c converted into c string and we need \0
    e2->allocate_and_set(&filter_filepath, &fp2[0], fp2.length() + 1);
    e3->allocate_and_set(&filter_filepath, &fp3[0], fp3.length() + 1);
    elements.push_back(e1);
    elements.push_back(e2);
    elements.push_back(e3);
    return elements;
}

void setup(JIT *jit) {
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
    ifft_fields.add(&ifft_offset1);
    ifft_fields.add(&ifft_offset2);
    ifft_fields.add(&clipped);

    FilterStage filt = create_filter_stage(jit, filter, "filter", &filter_fields);
    TransformStage reader = create_transform_stage(jit, read_audio, "read_audio", &filter_fields, &read_audio_fields);
    SegmentationStage segmentor = create_segmentation_stage(jit, segment, compute_num_segments,
                                                            "segment", "compute_num_segments",
                                                            &read_audio_fields, &segment_fields,
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
}

void start(JIT *jit) {
    std::vector<Element *> elements = init_filepaths();
    jit->dump();
    jit->add_module();
    run(jit, elements, &audio_filepath, &audio, &audio_length,
        &segment_filepath, &segment_audio, &segment_offset,
        &fft_filepath, &fft, &fft_offset,
        &ifft_filepath1, &ifft_filepath2,
        &ifft_offset1, &ifft_offset2, &clipped);
}

}