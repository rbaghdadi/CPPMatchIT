//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMSTAGE_H
#define MATCHIT_TRANSFORMSTAGE_H

#include "./Stage.h"
#include "./MFunc.h"
#include "./MType.h"
#include "./CodegenUtils.h"

template <typename I, typename O>
class TransformStage : public Stage {

private:

    O (*transform)(I);

public:

    TransformStage(O (*transform)(I), std::string transform_name, JIT *jit, MType *param_type, MType *return_type) :
            Stage(jit, mtype_of<I>(), mtype_of<O>(), transform_name), transform(transform) {
        std::vector<MType *> param_types;
        param_types.push_back(param_type);
        MType *stage_return_type = new MPointerType(new WrapperOutputType(return_type)); //create_type<WrapperOutput<create_type<O>()> *>();
        MFunc *func = new MFunc(function_name, "TransformStage", new MPointerType(return_type), stage_return_type, param_types, jit);
        set_function(func);
    }

    ~TransformStage() { }

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternArgLoaderIB *eal,
                                ExternCallIB *ec, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {

        // build the body
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

#endif //MATCHIT_TRANSFORMSTAGE_H
