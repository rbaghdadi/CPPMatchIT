//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

// TODO for now, we offload the writing of the segmentation to the user
template <typename I, typename O>
class SegmentationStage : Stage {

private:

    O* (*segment)(I);
//    unsigned int slide_amt;
//    unsigned int segment_size;
//    // creates a sliding window over a single input
//    ForLoop *slide_loop;
//    // creates the segment
//    ForLoop *segment_loop;

public:

    SegmentationStage(O* (segment)(I), std::string segmentation_name, JIT *jit) :
            Stage(jit, mtype_of<I>(), mtype_of<O*>(), segmentation_name), segment(segment){
        MType *ret_type = create_type<O*>();
        MType *arg_type = create_type<I>();
        std::vector<MType *> arg_types;
        arg_types.push_back(arg_type);
        MFunc *func = new MFunc(function_name, "SegmentationStage", ret_type, arg_types, jit);
        set_function(func);
        func->codegen_extern_proto();
        func->codegen_extern_wrapper_proto();
    }

    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb,
                                ExternCallBasicBlock *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
        // build the body
        // outer loop
        eibb->set_loop_idx(loop_idx);
        eibb->set_data(args[0]);
        eibb->codegen(jit, false);
        jit->get_builder().CreateBr(ecbb->get_basic_block());

        // create the call
        ecbb->set_extern_function(mfunction);
        std::vector<llvm::Value *> sliced;
        sliced.push_back(eibb->get_element());
        ecbb->set_extern_args(sliced);
        ecbb->codegen(jit, false);

        // we've now got a bunch of segments (array of SegmentTypes). flatten them out into a single stream
        // realloc if more space needed
        // They ret type of segment will be T**, AND we want the final array to be T**, so we need to keep reallocing
        // size (see CodegenUtils)

        jit->get_builder().CreateBr(branch_to);
    }

};


#endif //MATCHIT_SEGMENTATIONSTAGE_H
