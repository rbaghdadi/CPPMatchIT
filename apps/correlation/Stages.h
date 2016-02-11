//
// Created by Jessica Ray on 2/2/16.
//

#ifndef MATCHIT_STAGES_H
#define MATCHIT_STAGES_H

#include "../../src/ComparisonStage.h"
#include "../../src/TransformStage.h"
#include "../../src/SegmentationStage.h"
#include "../../src/Structures.h"

/*
 * Sliding window
 */

class SlidingWindow : public SegmentationStage<File *, Segments<float> *> {
private:

    static unsigned int segment_size;
    static unsigned int slide_amount;
    static unsigned int fft_size;

public:

    SlidingWindow(Segments<float> *(*segment)(File *), JIT *jit) :
            SegmentationStage(segment, "segment", jit, new FileType(), new SegmentsType(create_type<float>())) {}

    static unsigned int get_segment_size() {
        return segment_size;
    }

    static unsigned int get_slide_amount() {
        return slide_amount;
    }

    static unsigned int get_fft_size() {
        return fft_size;
    }

};

extern "C" Segments<float> *segment(File *filepath);

/*
 * FFT transform
 */

class FFTxform : public TransformStage<SegmentedElement<float> *, SegmentedElement<float> *> {
private:

public:

    FFTxform(SegmentedElement<float> *(*transform)(SegmentedElement<float> *), JIT *jit) :
            TransformStage(transform, "fft", jit, new SegmentedElementType(create_type<float>()),
                           new SegmentedElementType(create_type<float>())) {}

};

extern "C" SegmentedElement<float> *fft(SegmentedElement<float> *segment);
//
///*
// * iFFT transform
// */
//
//class iFFTxform : public ComparisonStage<SegmentedElement<float> *, ComparisonElement<float> *> {
//private:
//
//public:
//
//    iFFTxform(ComparisonElement<float> *(*transform)(SegmentedElement<float> *, SegmentedElement<float> *), JIT *jit) :
//            ComparisonStage(transform, "ifft", jit) {}
//
//};
//
//extern "C" ComparisonElement<float> *ifft(SegmentedElement<float> *segment1, SegmentedElement<float> *segment2);

#endif //MATCHIT_STAGES_H
