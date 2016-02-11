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

    SegmentationStage(O (*segment)(I), std::string segmentation_name, JIT *jit, MType *param_type, MType *return_type) :
            Stage(jit, mtype_of<I>(), mtype_of<O>(), segmentation_name), segment(segment) {
//        MType *return_type = create_type<O>();
//        MType *param_type = create_type<I>();
        std::vector<MType *> param_types;
        param_types.push_back(param_type);
        MType *segment_type = return_type->get_underlying_types()[0]->get_underlying_types()[0]->get_underlying_types()[0]->get_underlying_types()[0]->get_underlying_types()[0]; // this gets the type SegmentedElement<T>*
        segment_type->dump();
        MType *stage_return_type = new MPointerType(new WrapperOutputType(segment_type));
        MFunc *func = new MFunc(function_name, "SegmentationStage", new MPointerType(return_type), stage_return_type, param_types, jit);
        set_function(func);
    }

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternArgLoaderIB *eal,
                                ExternCallIB *ec, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
        // build the body
        // outer loop
        eal->set_mfunction(mfunction);
        eal->set_loop_idx_alloc(loop_idx);
        eal->set_wrapper_input_arg_alloc(args[0]);
        eal->codegen(jit, false);
        jit->get_builder().CreateBr(ec->get_basic_block());

        // create the call
        std::vector<llvm::AllocaInst *> sliced;
        sliced.push_back(eal->get_extern_input_arg_alloc());
        ec->set_mfunction(mfunction);
        ec->set_extern_arg_allocs(sliced);
        ec->codegen(jit, false);
        jit->get_builder().CreateBr(branch_to);
    }

};


#endif //MATCHIT_SEGMENTATIONSTAGE_H
