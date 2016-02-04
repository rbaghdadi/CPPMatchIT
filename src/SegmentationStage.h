//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

template <typename I, typename O>
class SegmentationStage : Stage {

private:

    O* (*segment)(I);

public:

    SegmentationStage(O* (segment)(I), std::string segmentation_name, JIT *jit) :
            Stage(jit, mtype_of<I>(), mtype_of<O*>(), segmentation_name), segment(segment) {
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


        // create the call
    }

};


#endif //MATCHIT_SEGMENTATIONSTAGE_H
