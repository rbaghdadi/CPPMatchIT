//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

class SegmentationStage : public Stage {
private:

    // Only here to enforce that the user's function signature is correct. And for debugging.
    unsigned int (*segment)(const Element * const, Element ** const) = nullptr;
    unsigned int (*compute_num_segments)(const Element * const) = nullptr;
    unsigned int segment_size;
    float overlap;
    BaseField *field_to_segment;
    std::string compute_num_segments_name;
    llvm::Function *compute_num_segments_mfunc;

public:

    // field_to_segment: the field that will actually be segmented
    SegmentationStage(unsigned int (*segment)(const Element *const, Element **const),
                      unsigned int (*compute_num_segments)(const Element *const), std::string segmentation_name,
                      std::string compute_num_segments_name, JIT *jit, Fields *input_relation, Fields *output_relation,
                      unsigned int segment_size, float overlap, BaseField *field_to_segment) :
            Stage(jit, "SegmentationStage", segmentation_name, input_relation, output_relation,
                  MScalarType::get_int_type()), segment(segment), compute_num_segments(compute_num_segments),
            segment_size(segment_size), overlap(overlap),
            field_to_segment(field_to_segment), compute_num_segments_name(compute_num_segments_name) {
        // trick compiler into thinking this is used
        (void)(this->segment);
        (void)(this->compute_num_segments);
        std::vector<MType *> seg_params;
        MType *element_type = create_type<Element>();
        MPointerType element_ptr = MPointerType(element_type);
        seg_params.push_back(&element_ptr);
        llvm::FunctionType *ft = llvm::FunctionType::get(MScalarType::get_int_type()->codegen_type(), element_ptr.codegen_type(), false);
        compute_num_segments_mfunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, compute_num_segments_name, jit->get_module().get());
    }

    bool is_segmentation();

    llvm::AllocaInst *compute_num_output_elements();

    void handle_user_function_output();

    std::string get_compute_num_segments_name() {
        return compute_num_segments_name;
    }

    llvm::Function *get_compute_num_segments_mfunc() {
        return compute_num_segments_mfunc;
    }

};

#endif //MATCHIT_SEGMENTATIONSTAGE_H
