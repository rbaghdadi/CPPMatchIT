//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMSTAGE_H
#define MATCHIT_TRANSFORMSTAGE_H

#include "Stage.h"
#include "./CodegenUtils.h"
#include "./MFunc.h"
#include "./MType.h"
#include "./ForLoop.h"
#include "./Utils.h"

template <typename I, typename O>
class TransformStage : public Stage {

private:

    // TODO this doesn't allow structs of structs...or does it?
    std::vector<MType *> input_struct_fields;
    std::vector<MType *> output_struct_fields;
    O (*transform)(I);

public:

    TransformStage(O (*transform)(I), std::string transform_name, JIT *jit, std::vector<MType *> input_struct_fields = std::vector<MType *>(),
                   std::vector<MType *> output_struct_fields = std::vector<MType *>()) :
            Stage(jit, mtype_of<I>(), mtype_of<O>(), transform_name), input_struct_fields(input_struct_fields),
            output_struct_fields(output_struct_fields), transform(transform) {
        MType *ret_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if (output_mtype_code == mtype_ptr && !output_struct_fields.empty()) {
            ret_type = create_struct_reference_type(output_struct_fields);
        } else if (output_mtype_code != mtype_struct) {
            ret_type = create_type<O>();
        } else {
            ret_type = create_struct_type(output_struct_fields);
        }

        MType *arg_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if(input_mtype_code == mtype_ptr && !input_struct_fields.empty()) {
            arg_type = create_struct_reference_type(input_struct_fields);
        } else if (input_mtype_code != mtype_struct) {
            arg_type = create_type<I>();
        } else {
            arg_type = create_struct_type(input_struct_fields);
        }

        std::vector<MType *> arg_types;
        arg_types.push_back(arg_type);
        MFunc *func = new MFunc(function_name, "TransformStage", ret_type, arg_types, jit);
        set_function(func);
        func->codegen_extern_proto();
        func->codegen_extern_wrapper_proto();
    }

    ~TransformStage() { }

    // TODO need either a copy constructor for MType since this is the same pointer getting copied over, or just a single MType copy, ever (like the llvm get function)
    TransformStage(const TransformStage &that) : Stage(that) {
        input_struct_fields = std::vector<MType *>(that.input_struct_fields);
        output_struct_fields = std::vector<MType *>(that.output_struct_fields);
        transform = that.transform;
    }

    // This is the place where the stage builds its function call based on the input arguments
    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb,
                                ExternCallBasicBlock *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
        // build the body
        eibb->set_loop_idx(loop_idx);//loop->get_loop_counter_basic_block()->get_loop_idx());
        eibb->set_data(args[0]);
        eibb->codegen(jit);
        jit->get_builder().CreateBr(ecbb->get_basic_block());

        // create the call
        ecbb->set_extern_function(mfunction);
        std::vector<llvm::Value *> sliced;
        sliced.push_back(eibb->get_element());
        ecbb->set_extern_args(sliced);
        ecbb->codegen(jit);
        jit->get_builder().CreateBr(branch_to);
    }

};

#endif //MATCHIT_TRANSFORMSTAGE_H
