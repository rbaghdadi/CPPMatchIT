//
// Created by Jessica Ray on 2/11/16.
//

#include "./JIT.h"
#include "./Utils.h"
#include "./MType.h"
#include "./LLVM.h"
#include "./CodegenUtils.h"
#include "./TransformStage.h"
#include "./FilterStage.h"
#include "./SegmentationStage.h"
#include "./Pipeline.h"
#include "./StageFactory.h"

extern "C" void identity_transform(const FloatElement *in, FloatElement *out) {
    out->set_tag(in->get_tag());
    out->set_data(in->get_data(), in->get_data_length());
    std::cerr << "the elements of this array are: ";
    float *f = out->get_data();
    for (int i = 0; i < out->get_data_length(); i++) {
        std::cerr << f[i] << " ";
    }
    std::cerr << std::endl;
}

extern "C" void truncate_fixed(const FloatElement *in, FloatElement *out) {
    out->set_tag(in->get_tag());
    out->set_data(in->get_data(), 5); // 5 is the transform size
    std::cerr << "truncated and now the elements of this array are: ";
    float *f = out->get_data();
    for (int i = 0; i < out->get_data_length(); i++) {
        std::cerr << f[i] << " ";
    }
    std::cerr << std::endl;
}

extern "C" bool my_filter(const FloatElement *in) {
    std::cerr << "filtering things" << std::endl;
    bool keep = in->get_data()[0] != 10.0;// keep the first thing
    std::cerr << "keep tag " << in->get_tag() << " " << keep << std::endl;
    return true;
}
//
//extern "C" void segmentor(const FloatElement *in, FloatSegment **out) {
//    std::cerr << "segmenting" << std::endl;
//    int seg_ctr = 0;
//    int num_segments = ceil((in->get_data_length() - (4*0.75)) / (4 - 4 * 0.75));
//    std::cerr << "num segments: " << num_segments << std::endl;
//    for (int i = 0; i < num_segments; i++) {
//        int start = i * 4 * 0.5;
//        std::cerr << "start: " << start << std::endl;
//        if ((start + 4) > in->get_data_length()) { // need to pad outwards
//            float padded[4];
//            int pad_amt = in->get_data_length() - (start + 4);
//            padded[0] = in->get_data(start);
//            if (pad_amt == 1) {
//                padded[1] = in->get_data(start + 1);
//                padded[2] = in->get_data(start + 1);
//                padded[3] = in->get_data(start + 2);
//            } else if (pad_amt == 2) {
//                padded[1] = in->get_data(start + 1);
//                padded[2] = in->get_data(start + 2);
//                padded[3] = in->get_data(start + 2);
//            } else if (pad_amt == 3) {
//                padded[1] = in->get_data(start + 2);
//                padded[2] = in->get_data(start + 2);
//                padded[3] = in->get_data(start + 2);
//            }
//            out[seg_ctr++]->set_data(padded, 4, start);
//        } else {
//            out[seg_ctr++]->set_data(&(in->get_data()[start]), 4, start);
//        }
//    }
//    std::cerr << "done segmenting" << std::endl;
//}

int seg_size = 4;

extern "C" void segmentor(const FloatElement *in, FloatSegment **out) {
    std::cerr << "segmenting" << std::endl;
    int seg_ctr = 0;
    int num_segments = ceil((in->get_data_length() - (seg_size*0.5)) / (seg_size - seg_size * 0.5));
    std::cerr << "num segments: " << num_segments << std::endl;
    for (int i = 0; i < num_segments; i++) {
        int start = i * seg_size * 0.5;
        std::cerr << "start: " << start << std::endl;
        if ((start + seg_size) > in->get_data_length()) { // need to pad outwards
            float padded[seg_size];
            int pad_amt = (start + seg_size) - in->get_data_length();
            std::cerr << "pad amount " << pad_amt << std::endl;
            // get the data that doesn't need padding
            int last_idx = 0;
            for (int j = start; j < in->get_data_length(); j++) {
                std::cerr << "j - start: " << (j - start) << std::endl;
                padded[j - start] = in->get_data(j);
                last_idx = j;
            }
            // get the padding
            int next_start = in->get_data_length();
            for (int j = last_idx; j < last_idx + pad_amt; j++) {
                padded[j] = in->get_data(next_start - 1);
                std::cerr << "j: " << j << std::endl;
            }//(start + seg_size - in->get_data_length()); j < )

            out[seg_ctr++]->set_data(padded, seg_size, start);
//            padded[0] = in->get_data(start);
//            if (pad_amt == 1) {
//                padded[1] = in->get_data(start + 1);
//                padded[2] = in->get_data(start + 1);
//                padded[3] = in->get_data(start + 2);
//            } else if (pad_amt == 2) {
//                padded[1] = in->get_data(start + 1);
//                padded[2] = in->get_data(start + 2);
//                padded[3] = in->get_data(start + 2);
//            } else if (pad_amt == 3) {
//                padded[1] = in->get_data(start + 2);
//                padded[2] = in->get_data(start + 2);
//                padded[3] = in->get_data(start + 2);
//            }
        } else {
            out[seg_ctr++]->set_data(&(in->get_data()[start]), seg_size, start);
        }
    }
    std::cerr << "done segmenting" << std::endl;
}


int main() {

    LLVM::init();
    JIT jit;
    register_utils(&jit);

    TransformStage<const FloatElement, FloatElement> trunc_matched =
            create_transform_stage(&jit, identity_transform, "identity_transform");

    TransformStage<const FloatElement, FloatElement> trunc_fixed =
            create_transform_stage(&jit, truncate_fixed, "truncate_fixed");

    FilterStage<const FloatElement> filter = create_filter_stage(&jit, my_filter, "my_filter");

    SegmentationStage<const FloatElement, FloatSegment> segment =
            create_segmentation_stage(&jit, segmentor, "segmentor", 4, 0.75);

    FloatElement *an_element = new FloatElement();
    an_element->set_tag(10L);
    float f1[] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};//, 9.0};//, 9.0, 9.0}; // padding for the segmentation
    an_element->set_data(f1, 9);

    FloatElement *another_element = new FloatElement();
    another_element->set_tag(29L);
    float f2[] = {10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0}; // padding for the segmentation
    another_element->set_data(f2, 10);

    std::vector<const void *> inputs;
    inputs.push_back(an_element);
    inputs.push_back(another_element);

    Pipeline pipeline;
    pipeline.register_stage(&filter);
//    pipeline.register_stage(&trunc_fixed);
    pipeline.register_stage(&segment);
    pipeline.codegen(&jit, 19, inputs.size());
    jit.dump();
    jit.add_module();
    for (int i = 0; i < 10; i++) {
        std::cerr << "Iteration: " << i << std::endl;
        pipeline.simple_execute(&jit, &(inputs[0]));
    }
}

//    // this says there are 10 total primitive values (floats in this case) across inputs.size() number of input structs
//    // it doesn't matter if this is fixed, matched, or variable size. It's just the raw total of prim values.
//    // If there is not a fixed size, then the preallocation code will need to be changed as follows:
//    // 1. pass in the sum of all the floats (or w/e your type is) in all the data arrays that are being passed in to the stage
//    // 2. allocate a block for all of those (don't multiply by the number of elements)
//    // 3. when looping through to chunk it up into the appropriate arrays, we will need to access the original input element
//    // that corresponds to the same index and read its data array size field. Then we use that.
//
//    // matched
//    // this says there are 20 total primitive values that will be passed along from input to output in the first stage
//    // TODO abstract this away from the user
//    pipeline.codegen(&jit, 24, inputs.size());
//    // fixed
//    // this says there are trunc_fixed.get_transform_size() * inputs.size() total primitive values that will be passed along
//    // from input to output in the first stage (so in this case of truncation, this value is NOT the number of primitive values
//    // in the input. It is the number that should be in the output)
////    pipeline.codegen(&jit, trunc_fixed.get_transform_size() * inputs.size(), inputs.size());
//
//    jit.dump();
//
//
//    jit.add_module();
//    for (int i = 0; i < 10; i++) {
//        std::cerr << "Iteration: " << i << std::endl;
//        pipeline.simple_execute(&jit, &(inputs[0]));
//    }

//}