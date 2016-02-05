//
// Created by Jessica Ray on 2/2/16.
//

#include <fftw3.h>
#include "./Stages.h"
#include "../file_dups/Stages.h"
#include "Stages.h"

/*
 * Apply function definitions
 */
//
//extern "C" Segment *segment(float *data) {
//    unsigned int slide_amt;
//    unsigned int data_size;
//}
//
//extern "C" Segment *fft(Segment *segment) {
//    Segment *output = (Segment *)fftwf_malloc(sizeof(*output));
//    output->filename = (char *)fftwf_malloc(sizeof(*(output->filename)) * 256);
//    output->segment = fftwf_alloc_real(FFTxform::get_fft_size());
//
//    float *input = segment->segment;
//    fftwf_plan plan = fftwf_plan_r2r_1d(FFTxform::get_fft_size(), input, output->segment, FFTW_R2HC, FFTW_ESTIMATE);
//    fftwf_execute(plan);
//    fftwf_destroy_plan(plan);
//
//    return output;
//}
//
//extern "C" Comparison *ifft(Segment *segment1, Segment *segment2) {
//    Comparison *output = (Comparison *)fftwf_malloc(sizeof(Comparison));
//    output->filename1 = (char *)fftwf_malloc(sizeof(*(output->filename1)) * 256);
//    output->filename2 = (char *)fftwf_malloc(sizeof(*(output->filename2)) * 256);
//    output->correlation = fftwf_alloc_real(FFTxform::get_fft_size() - iFFTxform::get_segment_size() + 2);
//
//    // complex multiply
//    float* convolved = fftwf_alloc_real(FFTxform::get_fft_size());
//    for (size_t real_idx = 1; real_idx < FFTxform::get_fft_size() / 2; real_idx++) {
//        unsigned int imag_idx = FFTxform::get_fft_size() - real_idx;
//        float real = segment1->segment[real_idx] * segment2->segment[real_idx] -
//                     segment1->segment[imag_idx] * segment2->segment[imag_idx];
//        float imag = segment1->segment[real_idx] * segment2->segment[imag_idx] +
//                     segment1->segment[imag_idx] * segment2->segment[real_idx];
//        convolved[real_idx] = real;
//        convolved[imag_idx] = imag;
//    }
//    convolved[0] = segment1->segment[0] * segment2->segment[0];
//    convolved[FFTxform::get_fft_size() / 2] = segment1->segment[FFTxform::get_fft_size() / 2] * segment2->segment[FFTxform::get_fft_size() / 2];
//
//    // iFFT
//    float* ifft_out = fftwf_alloc_real(FFTxform::get_fft_size());
//    fftwf_plan c2r_plan = fftwf_plan_r2r_1d(FFTxform::get_fft_size(), convolved, ifft_out, FFTW_HC2R, FFTW_ESTIMATE);
//    fftwf_execute(c2r_plan);
//    fftwf_free(convolved);
//
//    // clip (threshold) the iFFT output
//    unsigned int cur_output_idx = 0;
//    for (size_t i = 0; i < FFTxform::get_fft_size(); i++) {
//        if (i > iFFTxform::get_segment_size() - 2) {
//            float clipped = ifft_out[i] / (float)FFTxform::get_fft_size() * 32767.0;
//            if (clipped > 32767.0)
//                clipped = 32767;
//            else if (clipped < -32767)
//                clipped = -32767;
//            output->correlation[cur_output_idx++] = clipped;
//        }
//    }
//
//    fftwf_free(ifft_out);
//    fftwf_destroy_plan(c2r_plan);
//}
