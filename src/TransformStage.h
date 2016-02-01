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
//    ForLoop *loop;

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

//        loop = new ForLoop(jit, func);
    }

    ~TransformStage() { }


    // This is the place where the stage builds its function call based on the input arguments
    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb, ExternCallBasicBlock *ecbb, ExternCallStoreBasicBlock *ecsbb) {
        // build the body
        eibb->set_function(mfunction->get_extern_wrapper());
        eibb->set_loop_idx(loop->get_loop_counter_basic_block()->get_loop_idx());
        eibb->set_data(args[0]);
        eibb->codegen(jit);
        jit->get_builder().CreateBr(ecbb->get_basic_block());

        // create the call
        ecbb->set_function(mfunction->get_extern_wrapper());
        ecbb->set_extern_function(mfunction);
        std::vector<llvm::Value *> sliced;
        sliced.push_back(eibb->get_element());
        ecbb->set_extern_args(sliced);
        ecbb->codegen(jit);
//        postprocess(this, ecbb->get_basic_block(), extern_call_store_basic_block->get_basic_block());
        jit->get_builder().CreateBr(ecsbb->get_basic_block());
    }

    void codegen() {
        // initialize the function args
        extern_arg_prep_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_arg_prep_basic_block->set_extern_function(mfunction);
        extern_arg_prep_basic_block->codegen(jit);
        jit->get_builder().CreateBr(loop->get_loop_counter_basic_block()->get_basic_block());

        // loop components
        loop->set_max_loop_bound(llvm::cast<llvm::AllocaInst>(last(extern_arg_prep_basic_block->get_args())));
        loop->set_branch_to_after_counter(return_struct_basic_block->get_basic_block());
        loop->set_branch_to_true_condition(extern_init_basic_block->get_basic_block());
        loop->set_branch_to_false_condition(for_loop_end_basic_block->get_basic_block());
        loop->codegen();

        // allocate space for the return structure
        return_struct_basic_block->set_function(mfunction->get_extern_wrapper());
        return_struct_basic_block->set_extern_function(mfunction);
        return_struct_basic_block->set_max_num_ret_elements(loop->get_loop_counter_basic_block()->get_loop_bound());
        return_struct_basic_block->set_stage_return_type(mfunction->get_extern_wrapper_data_ret_type());
        return_struct_basic_block->codegen(jit);
        jit->get_builder().CreateBr(loop->get_for_loop_condition_basic_block()->get_basic_block());

        // build the body
        extern_init_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_init_basic_block->set_loop_idx(loop->get_loop_counter_basic_block()->get_loop_idx());
        extern_init_basic_block->set_data(llvm::cast<llvm::AllocaInst>(extern_arg_prep_basic_block->get_args()[0]));
        extern_init_basic_block->codegen(jit);
        jit->get_builder().CreateBr(extern_call_basic_block->get_basic_block());

        // create the call
        extern_call_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_call_basic_block->set_extern_function(mfunction);
        std::vector<llvm::Value *> sliced;
        sliced.push_back(extern_init_basic_block->get_element());
        extern_call_basic_block->set_extern_args(sliced);
        extern_call_basic_block->codegen(jit);
        postprocess(this, extern_call_basic_block->get_basic_block(), extern_call_store_basic_block->get_basic_block());

        // store the result
        extern_call_store_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_call_store_basic_block->set_mtype(mfunction->get_extern_wrapper_data_ret_type());
        extern_call_store_basic_block->set_data_to_store(extern_call_basic_block->get_data_to_return());
        extern_call_store_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
        extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
        extern_call_store_basic_block->codegen(jit);
        jit->get_builder().CreateBr(loop->get_for_loop_increment_basic_block()->get_basic_block());

        // return the data
        for_loop_end_basic_block->set_function(mfunction->get_extern_wrapper());
        for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
        for_loop_end_basic_block->set_return_idx(loop->get_loop_counter_basic_block()->get_return_idx());
        for_loop_end_basic_block->codegen(jit);

        mfunction->verify_wrapper();
    }
//
//    void codegen_OLD() {
//        // initialize the function args
//        extern_arg_prep_basic_block->set_function(mfunction->get_extern_wrapper());
//        extern_arg_prep_basic_block->set_extern_function(mfunction);
//        extern_arg_prep_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(loop_counter_basic_block->get_basic_block());
//
//        // allocate space for the loop bounds and counters
//        loop_counter_basic_block->set_function(mfunction->get_extern_wrapper());
//        loop_counter_basic_block->set_max_bound(extern_arg_prep_basic_block->get_args()[1]);
//        loop_counter_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(return_struct_basic_block->get_basic_block());
//
//        // allocate space for the return structure
//        return_struct_basic_block->set_function(mfunction->get_extern_wrapper());
//        return_struct_basic_block->set_extern_function(mfunction);
//        return_struct_basic_block->set_loop_bound(loop_counter_basic_block->get_loop_bound());
//        return_struct_basic_block->set_stage_return_type(mfunction->get_extern_wrapper_data_ret_type());
//        return_struct_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());
//
//        // loop condition
//        for_loop_condition_basic_block->set_function(mfunction->get_extern_wrapper());
//        for_loop_condition_basic_block->set_max_bound(loop_counter_basic_block->get_loop_bound());
//        for_loop_condition_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
//        for_loop_condition_basic_block->codegen(jit);
//        jit->get_builder().CreateCondBr(for_loop_condition_basic_block->get_loop_comparison(), extern_init_basic_block->get_basic_block(), for_loop_end_basic_block->get_basic_block());
//
//        // loop index increment
//        for_loop_increment_basic_block->set_function(mfunction->get_extern_wrapper());
//        for_loop_increment_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
//        for_loop_increment_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());
//
//        // build the body
//        extern_init_basic_block->set_function(mfunction->get_extern_wrapper());
//        extern_init_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
//        extern_init_basic_block->set_data(llvm::cast<llvm::AllocaInst>(extern_arg_prep_basic_block->get_args()[0]));
//        extern_init_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(extern_call_basic_block->get_basic_block());
//
//        // create the call
//        extern_call_basic_block->set_function(mfunction->get_extern_wrapper());
//        extern_call_basic_block->set_extern_function(mfunction);
//        std::vector<llvm::Value *> sliced;
//        sliced.push_back(extern_init_basic_block->get_element());
//        extern_call_basic_block->set_extern_args(sliced);
//        extern_call_basic_block->codegen(jit);
//        postprocess(this, extern_call_basic_block->get_basic_block(), extern_call_store_basic_block->get_basic_block());
//
//        // store the result
//        extern_call_store_basic_block->set_function(mfunction->get_extern_wrapper());
//        extern_call_store_basic_block->set_mtype(mfunction->get_extern_wrapper_data_ret_type());
//        extern_call_store_basic_block->set_data_to_store(extern_call_basic_block->get_data_to_return());
//        extern_call_store_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
//        extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
//        extern_call_store_basic_block->codegen(jit);
//        jit->get_builder().CreateBr(for_loop_increment_basic_block->get_basic_block());
//
//        // return the data
//        for_loop_end_basic_block->set_function(mfunction->get_extern_wrapper());
//        for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
//        for_loop_end_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
//        for_loop_end_basic_block->codegen(jit);
//
//        mfunction->verify_wrapper();
//    }

    void postprocess(Stage *stage, llvm::BasicBlock *branch_from, llvm::BasicBlock *branch_into) {
        jit->get_builder().SetInsertPoint(branch_from);
        jit->get_builder().CreateBr(branch_into);
    }

};

#endif //MATCHIT_TRANSFORMSTAGE_H
