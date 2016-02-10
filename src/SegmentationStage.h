//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

// TODO for now, we offload the writing of the segmentation to the user
// TODO enforce that the output O must be of type Segments
template <typename I, typename O>
class SegmentationStage : public Stage {

private:

    O (*segment)(I);

public:

    SegmentationStage(O (*segment)(I), std::string segmentation_name, JIT *jit) :
            Stage(jit, mtype_of<I>(), mtype_of<O>(), segmentation_name), segment(segment) {
        MType *return_type = create_type<O>();
        MType *param_type = create_type<I>();
        std::vector<MType *> param_types;
        param_types.push_back(param_type);
        // the extern_wrapper_return_type in this case is an marray of type SegmentedElement.
        MFunc *func = new MFunc(function_name, "SegmentationStage", return_type->get_underlying_types()[0], return_type, param_types, jit);
        set_function(func);
    }

    void init_codegen() {
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
