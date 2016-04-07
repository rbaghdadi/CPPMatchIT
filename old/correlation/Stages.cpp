//
// Created by Jessica Ray on 2/2/16.
//

#include <fftw3.h>
#include <float.h>
#include "./Stages.h"
#include "../file_dups/Stages.h"

/*
 * Apply function definitions
 */

unsigned int SlidingWindow::segment_size = 44335;
unsigned int SlidingWindow::slide_amount = 22168;
unsigned int SlidingWindow::fft_size = 65536;

extern "C" Segments<float> *segment(File *filepath) {
    // read the audio data in
    FILE* audio = fopen(filepath->get_filepath()->get_array(), "rb");
    fseek(audio, 0L, SEEK_END);
    int audio_len = ftell(audio) / sizeof(short);
    fseek(audio, 0L, SEEK_SET);
    // it's okay to use a c array here because it's just an intermediate--won't actually be returned from here
    short *audio_data = (short*)malloc(sizeof(short) * audio_len);
    fread(audio_data, sizeof(short), audio_len, audio);
    fclose(audio);
    // scale the audio
    float *scaled_audio;
    scaled_audio = fftwf_alloc_real(audio_len); // 16 byte aligned
    double scale = 0.0;
    for (ptrdiff_t i = audio_len - 1; i >= 0; i--)
        scale += audio_data[i] * audio_data[i];
    scale = sqrt(scale);
    scale = (scale < 1.0 / FLT_MAX) ? 0.0 : 0.5 / scale;
    for (size_t i = 0; i < audio_len; i++) {
        scaled_audio[i] = (float)audio_data[i] * (float)scale;
    }
    free(audio_data);

    // initialize the overall Segments structure
    Segments<float> *segments = (Segments<float> *)malloc(sizeof(Segments<float>));
    unsigned int number_of_segments = ceil((float)audio_len / (float)SlidingWindow::get_slide_amount());
    MArray<SegmentedElement<float> *> *marray_segments = new MArray<SegmentedElement<float> *>(number_of_segments);
    segments->set_segments(marray_segments);

    // now do the segmenting
    std::cerr << "Number of segments in file " << filepath->get_filepath()->get_array() << " is " << number_of_segments << std::endl;
    for (size_t block_start = 0; block_start < audio_len; block_start+=SlidingWindow::get_slide_amount()) {
        unsigned int block_end = block_start + SlidingWindow::get_segment_size() < audio_len ? block_start +
                                                                                               SlidingWindow::get_segment_size() : audio_len - 1;
        SegmentedElement<float> *segment = (SegmentedElement<float> *)fftwf_malloc(sizeof(SegmentedElement<float>));
        segment->set_filepath(new MArray<char>(filepath->get_filepath()->get_array(), filepath->get_filepath()->get_malloc_size()));
        MArray<float> *one_segment = new MArray<float>(SlidingWindow::get_fft_size());
        segment->set_offset(block_start);
        for (unsigned int i = block_start; i < block_end; i++) {
            one_segment->add(scaled_audio[i]);
        }
        // pad any remaining with 0s
        for (size_t i = 0; i < SlidingWindow::get_fft_size() - (block_end - block_start); i++) {
            one_segment->add(0.0);
        }
        segment->set_data(one_segment);
        segments->add_segment(segment);
    }
    free(scaled_audio);
    return segments;
}

extern "C" SegmentedElement<float> *fft(SegmentedElement<float> *segment) {
    std::cerr << "Running FFT on file: " << segment->get_filepath()->get_array() << " with offset " << segment->get_offset() << std::endl;
    SegmentedElement<float> *xformed = (SegmentedElement<float> *)fftwf_malloc(sizeof(SegmentedElement<float>));
    xformed->set_filepath(new MArray<char>(segment->get_filepath()->get_array(), segment->get_filepath()->get_malloc_size()));
    MArray<float> *fft_seg = new MArray<float>(SlidingWindow::get_fft_size());
    xformed->set_data(fft_seg);

//    xformed->filepath = new MArray<char>(segment->filepath->get_num_elements());
//    xformed->data = new MArray<float>(SlidingWindow::get_fft_size());
    float *input = segment->get_data()->get_array();
    // fft input and store in xformed data MArray
    fftwf_plan plan = fftwf_plan_r2r_1d(SlidingWindow::get_fft_size(), input, xformed->get_data()->get_array(), FFTW_R2HC, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
    return xformed;
}
//
//extern "C" ComparisonElement<float> *ifft(SegmentedElement<float> *segment1, SegmentedElement<float> *segment2) {
//    ComparisonElement<float> *output = (ComparisonElement<float> *)fftwf_malloc(sizeof(ComparisonElement<float>));
//    output->filepath1 = new MArray<char>(segment1->filepath->get_num_elements());
//    output->filepath2 = new MArray<char>(segment2->filepath->get_num_elements());
//    output->comparison = new MArray<float>(FFTxform::get_fft_size() - SlidingWindow::get_segment_size() + 2);
//
//    // complex multiply
//    float* convolved = fftwf_alloc_real(FFTxform::get_fft_size());
//    for (size_t real_idx = 1; real_idx < FFTxform::get_fft_size() / 2; real_idx++) {
//        unsigned int imag_idx = FFTxform::get_fft_size() - real_idx;
//        float real = segment1->data->get_underlying_array()[real_idx] * segment2->data->get_underlying_array()[real_idx] -
//                     segment1->data->get_underlying_array()[imag_idx] * segment2->data->get_underlying_array()[imag_idx];
//        float imag = segment1->data->get_underlying_array()[real_idx] * segment2->data->get_underlying_array()[imag_idx] +
//                     segment1->data->get_underlying_array()[imag_idx] * segment2->data->get_underlying_array()[real_idx];
//        convolved[real_idx] = real;
//        convolved[imag_idx] = imag;
//    }
//    convolved[0] = segment1->data->get_underlying_array()[0] * segment2->data->get_underlying_array()[0];
//    convolved[FFTxform::get_fft_size() / 2] = segment1->data->get_underlying_array()[FFTxform::get_fft_size() / 2] *
//                                              segment2->data->get_underlying_array()[FFTxform::get_fft_size() / 2];
//
//    // iFFT
//    float* ifft_out = fftwf_alloc_real(FFTxform::get_fft_size());
//    fftwf_plan c2r_plan = fftwf_plan_r2r_1d(FFTxform::get_fft_size(), convolved, ifft_out, FFTW_HC2R, FFTW_ESTIMATE);
//    fftwf_execute(c2r_plan);
//    fftwf_free(convolved);
//
//    // clip (threshold) the iFFT output
////    unsigned int cur_output_idx = 0;
//    for (size_t i = 0; i < FFTxform::get_fft_size(); i++) {
//        if (i > SlidingWindow::get_segment_size() - 2) {
//            float clipped = ifft_out[i] / (float)FFTxform::get_fft_size() * 32767.0;
//            if (clipped > 32767.0)
//                clipped = 32767;
//            else if (clipped < -32767)
//                clipped = -32767;
////            output->comparison->get_underlying_array()[cur_output_idx++] = clipped;
//            output->comparison->add(clipped);
//        }
//    }
//
//    fftwf_free(ifft_out);
//    fftwf_destroy_plan(c2r_plan);
//}
