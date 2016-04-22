//
// Created by Jessica Ray on 4/22/16.
//

#ifndef MATCHIT_MF_H
#define MATCHIT_MF_H

#include "../../src/Field.h"

namespace MF {

const int fft_size = 65536;
const int seg_size = 75000;
const float overlap = 0.00;
const int dim = 45539;

/**
 * FilterStage. Remove all files that end in txt
 */
extern Field<char, 200> filter_filepath;
extern "C" bool filter(const Element * const input);

/**
 * TransformationStage. Reads in audio.
 */
extern Field<char, 200> audio_filepath;
extern Field<float, 1500000> audio;
extern Field<int> audio_length; // This is needed since we don't have variable size arrays. It's the actual length of the audio
extern "C" void read_audio(const Element * const in, Element * const out);

/**
 * SegmentationStage. Segment the audio into fixed size chunks.
 */
extern Field<char, 200> segment_filepath;
extern Field<float, seg_size> segment_audio;
extern Field<int> segment_offset;
extern "C" unsigned int segment(const Element * const audio_in, Element ** const segmented_audio);

/**
 * TransformStage. Compute the FFT of the chunks.
 */
extern Field<char, 200> fft_filepath;
extern Field<float, fft_size> fft;
extern Field<int> fft_offset;
extern "C" void compute_fft(const Element * const input, Element * const output);

/**
 * ComparisonStage. Correlate the FFTs by multiplying and then applying the inverse FFT.
 */
extern Field<char, 200> ifft_filepath1;
extern Field<char, 200> ifft_filepath2;
extern Field<float, dim> clipped;
// http://dsp.stackexchange.com/questions/736/how-do-i-implement-cross-correlation-to-prove-two-audio-files-are-similar
// In Doug's original code, the pipeline reverses the referent, takes the FFT of that, and then does this computation with
// FFTs of the signal (not reversed). I can't really do the reversal here because I would need to do that in the compute_fft stage,
// and then keep track of which signals are reversed and such so that I don't compare 2 reversed signals. But,
// the link above shows that I can do the reversal after taking the FFT by taking the complex conjugate. This means I can just do that
// to one of the signals in the compute_ifft function. I don't think it matters which one, so just pick fft1.
// TODO I'm not actually doing this yet
extern "C" bool compute_ifft(const Element * const fft1, const Element * const fft2, Element * const ifft_out);

// for reduction
extern "C" int map(const Element * const ifft_result);

/**
 * Create input Elements with filepaths.
 */
std::vector<Element *> init_filepaths();

void setup(JIT *jit);

void start(JIT *jit);

}

#endif //MATCHIT_MF_H
