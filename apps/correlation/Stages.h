//
// Created by Jessica Ray on 2/2/16.
//

#ifndef MATCHIT_STAGES_H
#define MATCHIT_STAGES_H

#include "../../src/ComparisonStage.h"
#include "../../src/TransformStage.h"
#include "../../src/SegmentationStage.h"
#include "../../src/CompositeTypes.h"

class SlidingWindow : public SegmentationStage<Element<float> *, SegmentedElement<float> *> {

    static unsigned int segment_size;

public:

    SlidingWindow(SegmentedElement<float> **(*segment)(Element<float> *), JIT *jit) :
            SegmentationStage(segment, "segment", jit) {}

    static unsigned int get_segment_size() {
        return segment_size;
    }

};

unsigned int SlidingWindow::segment_size = 10;

extern "C" SegmentedElement<float> **segment(Element<float> *element);

//
//typedef struct {
//    /*
//     * offset of segment into file specified by filename
//     */
//    unsigned int offset;
//    /*
//     * segment originates from this filename
//     */
//    char *filename;
//    /*
//     * all the data for this segment
//     */
//    float *segment;
//} Segment;
//
//typedef struct {
//    /*
//     * 1st segment compared originates from this filename
//     */
//    char *filename1;
//    /*
//     * 1st segment compared originates from this filename
//     */
//    char *filename2;
//    /*
//     * offset of 1st segment into file specified by filename1
//     */
//    unsigned int offset_filename1;
//    /*
//     * offset of 2nd segment into file specified by filename2
//     */
//    unsigned int offset_filename2;
//    /*
//     * output of the iFFT
//     */
//    float *correlation;
//} Comparison;
//
//class FFTxform : public TransformStage<Segment *, Segment *> {
//private:
//
//    static unsigned int fft_size;
//
//public:
//
//    FFTxform(Segment *(*transform)(Segment *), JIT *jit, std::vector<MType *> segment_fields) :
//            TransformStage(transform, "fft", jit, segment_fields, segment_fields) {}
//
//    static unsigned int get_fft_size() {
//        return fft_size;
//    }
//
//    static void set_fft_size(unsigned int _fft_size) {
//        fft_size = _fft_size;
//    }
//
//};
//
//extern "C" Segment *fft(Segment *segment);
//
//class iFFTxform : public ComparisonStage<Segment *, Comparison *> {
//private:
//
//    static unsigned int segment_size;
//    static unsigned int fft_size;
//
//public:
//
//    iFFTxform(Comparison *(*transform)(Segment *, Segment *), JIT *jit, std::vector<MType *> segment_fields, std::vector<MType *> comparison_fields) :
//            ComparisonStage(transform, "ifft", jit, segment_fields, comparison_fields) {}
//
//    static unsigned int get_fft_size() {
//        return fft_size;
//    }
//
//    static void set_segment_size(unsigned int _segment_size) {
//        segment_size = _segment_size;
//    }
//
//    static void set_fft_size(unsigned int _fft_size) {
//        fft_size = _fft_size;
//    }
//
//    static unsigned int get_segment_size() {
//        return segment_size;
//    }
//
//};
//
//extern "C" Comparison *ifft(Segment *segment1, Segment *segment2);

#endif //MATCHIT_STAGES_H
