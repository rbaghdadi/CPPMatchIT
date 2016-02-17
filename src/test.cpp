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
#include "./Pipeline.h"

class TruncateTransform : public TransformStage<const Element2<float>, Element2<float>> {

public:

    TruncateTransform(void (*transform)(const Element2<float>*, Element2<float>*), JIT *jit) :
            TransformStage(transform, "my_truncate_matched", jit, new ElementType(create_type<float>()), new ElementType(create_type<float>()), 0, false) { }
//     TransformStage(transform, "my_truncate_fixed", jit, new ElementType(create_type<float>()), new ElementType(create_type<float>()), 5, true) {}
};

class Filter : public FilterStage<const Element2<float>> {

public:

    Filter(bool (*filter)(const Element2<float>*), JIT *jit) : FilterStage(filter, "my_filter", jit,
                                                                           new ElementType(create_type<float>())) { }

};

// for the matched size
extern "C" void my_truncate_matched(const Element2<float> *in, Element2<float> *out) {
    std::cerr << "in here" << std::endl;
    out->set_tag(in->get_tag());
    out->set_data(in->get_data(), in->get_data_length());
    std::cerr << "done and the elements of this array are: ";
    float *f = out->get_data();
    for (int i = 0; i < out->get_data_length(); i++) {
        std::cerr << f[i] << " ";
    }
    std::cerr << std::endl;
}

// for the fixed size
extern "C" void my_truncate_fixed(const Element2<float> *in, Element2<float> *out) {
    std::cerr << "in here" << std::endl;
    out->set_tag(in->get_tag());
    out->set_data(in->get_data(), 5); // 5 is the transform size
    std::cerr << "done and the elements of this array are: ";
    float *f = out->get_data();
    for (int i = 0; i < out->get_data_length(); i++) {
        std::cerr << f[i] << " ";
    }
    std::cerr << std::endl;
}

extern "C" bool my_filter(const Element2<float> *in) {
    std::cerr << "fake filtering things" << std::endl;
    return true;
}

int main() {

    LLVM::init();
    JIT jit;
    register_utils(&jit);

    TruncateTransform trunc_matched(my_truncate_matched, &jit);
    TruncateTransform trunc_fixed(my_truncate_fixed, &jit);
    Filter filter(my_filter, &jit);


    Element2<float> *an_element = new Element2<float>();
    an_element->set_tag(10L);
    float f1[] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
    an_element->set_data(f1, 10);

    Element2<float> *another_element = new Element2<float>();
    another_element->set_tag(29L);
    float f2[] = {10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0};
    another_element->set_data(f2, 10);

    std::vector<const void *> inputs;
    inputs.push_back(an_element);
    inputs.push_back(another_element);

    Pipeline pipeline;
//    pipeline.register_stage(&trunc_matched);
    pipeline.register_stage(&filter);
    // this says there are 10 total primitive values (floats in this case) across inputs.size() number of input structs
    // it doesn't matter if this is fixed, matched, or variable size. It's just the raw total of prim values.
    // If there is not a fixed size, then the preallocation code will need to be changed as follows:
    // 1. pass in the sum of all the floats (or w/e your type is) in all the data arrays that are being passed in to the stage
    // 2. allocate a block for all of those (don't multiply by the number of elements)
    // 3. when looping through to chunk it up into the appropriate arrays, we will need to access the original input element
    // that corresponds to the same index and read its data array size field. Then we use that.

    // matched
    // this says there are 20 total primitive values that will be passed along from input to output in the first stage
    // TODO abstract this away from the user
    pipeline.codegen(&jit, 20, inputs.size());
    // fixed
    // this says there are trunc_fixed.get_transform_size() * inputs.size() total primitive values that will be passed along
    // from input to output in the first stage (so in this case of truncation, this value is NOT the number of primitive values
    // in the input. It is the number that should be in the output)
//    pipeline.codegen(&jit, trunc_fixed.get_transform_size() * inputs.size(), inputs.size());

    jit.dump();


    jit.add_module();
    for (int i = 0; i < 10; i++)
        pipeline.simple_execute(&jit, &(inputs[0]));

}