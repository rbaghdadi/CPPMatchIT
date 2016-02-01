//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMSTAGE_H
#define MATCHIT_TRANSFORMSTAGE_H

#include "Stage.h"
#include "./CodegenUtils.h"
#include "./MFunc.h"
#include "./MType.h"

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
        // TODO what if input type is a struct (see ComparisonBlock)
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

    void codegen() {
//        mfunction->codegen_extern_wrapper_proto();
        // initialize the function args
        extern_arg_prep_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_arg_prep_basic_block->set_extern_function(mfunction);
        extern_arg_prep_basic_block->codegen(jit);
        jit->get_builder().CreateBr(loop_counter_basic_block->get_basic_block());

        // allocate space for the loop bounds and counters
        loop_counter_basic_block->set_function(mfunction->get_extern_wrapper());
        loop_counter_basic_block->set_max_bound(extern_arg_prep_basic_block->get_args()[1]);
        loop_counter_basic_block->codegen(jit);
        jit->get_builder().CreateBr(return_struct_basic_block->get_basic_block());

        // allocate space for the return structure
        return_struct_basic_block->set_function(mfunction->get_extern_wrapper());
        return_struct_basic_block->set_extern_function(mfunction);
        return_struct_basic_block->set_loop_bound(loop_counter_basic_block->get_loop_bound());
        return_struct_basic_block->set_stage_return_type(mfunction->get_extern_wrapper_data_ret_type());
        return_struct_basic_block->codegen(jit);
        jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());

        // loop condition
        for_loop_condition_basic_block->set_function(mfunction->get_extern_wrapper());
        for_loop_condition_basic_block->set_loop_bound(loop_counter_basic_block->get_loop_bound());
        for_loop_condition_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
        for_loop_condition_basic_block->codegen(jit);
        jit->get_builder().CreateCondBr(for_loop_condition_basic_block->get_loop_comparison(), extern_init_basic_block->get_basic_block(), for_loop_end_basic_block->get_basic_block());

        // loop index increment
        for_loop_increment_basic_block->set_function(mfunction->get_extern_wrapper());
        for_loop_increment_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
        for_loop_increment_basic_block->codegen(jit);
        jit->get_builder().CreateBr(for_loop_condition_basic_block->get_basic_block());

        // build the body
        extern_init_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_init_basic_block->set_loop_idx(loop_counter_basic_block->get_loop_idx());
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
//        jit->get_builder().CreateBr(extern_call_store_basic_block->get_basic_block());
//        dummy_block->insertInto(mfunction->get_extern_wrapper());
//        jit->get_builder().CreateBr(dummy_block);
//        jit->get_builder().SetInsertPoint(dummy_block);
//        jit->get_builder().CreateBr(extern_call_store_basic_block->get_basic_block());
//        jit->get_builder().SetInsertPoint(extern_call_basic_block->get_basic_block());
        postprocess(this, extern_call_basic_block->get_basic_block(), extern_call_store_basic_block->get_basic_block());

        // store the result
        extern_call_store_basic_block->set_function(mfunction->get_extern_wrapper());
        extern_call_store_basic_block->set_mtype(mfunction->get_extern_wrapper_data_ret_type());
        extern_call_store_basic_block->set_data_to_store(extern_call_basic_block->get_data_to_return());
        extern_call_store_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
        extern_call_store_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
        extern_call_store_basic_block->codegen(jit);
        jit->get_builder().CreateBr(for_loop_increment_basic_block->get_basic_block());

        // return the data
        for_loop_end_basic_block->set_function(mfunction->get_extern_wrapper());
        for_loop_end_basic_block->set_return_struct(return_struct_basic_block->get_return_struct());
        for_loop_end_basic_block->set_return_idx(loop_counter_basic_block->get_return_idx());
        for_loop_end_basic_block->codegen(jit);

        mfunction->verify_wrapper();
    }

    void postprocess(Stage *stage, llvm::BasicBlock *branch_from, llvm::BasicBlock *branch_into) {
//        jit->get_builder().CreateBr(extern_call_store_basic_block->get_basic_block());
//        jit->get_builder().CreateBr(dummy_block);
        jit->get_builder().SetInsertPoint(branch_from);
        jit->get_builder().CreateBr(branch_into);//stage->get_extern_call_store_basic_block()->get_basic_block());
    }

    void codegen_old() {
        // TODO what if input type is a struct (see ComparisonBlock)
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
        set_function(mfunction);

        func->codegen_extern_proto();
        func->codegen_extern_wrapper_proto();

        using namespace CodegenUtils;

        llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", func->get_extern_wrapper());
        llvm::BasicBlock *for_cond = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.cond", func->get_extern_wrapper());
        llvm::BasicBlock *for_inc = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.inc", func->get_extern_wrapper());
        llvm::BasicBlock *for_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.body", func->get_extern_wrapper());
        llvm::BasicBlock *for_extern_call = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.extern_call", func->get_extern_wrapper());
        llvm::BasicBlock *for_end = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.end", func->get_extern_wrapper());
        llvm::BasicBlock *store_data = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store", func->get_extern_wrapper());

        // initialize stuff
        // counters
        jit->get_builder().SetInsertPoint(entry);
        FuncComp loop_idx = init_idx(jit);
        FuncComp ret_idx = init_idx(jit);
        FuncComp args = init_function_args(jit, *func);
        // return structure
        FuncComp ret_struct = init_return_data_structure(jit, ret_type, *func, args.get_last());
        jit->get_builder().CreateBr(for_cond);

        // create the for loop parts
        jit->get_builder().SetInsertPoint(for_cond);
        FuncComp loop_condition = create_loop_idx_condition(jit, llvm::cast<llvm::AllocaInst>(loop_idx.get_result(0)),
                                                            llvm::cast<llvm::AllocaInst>(args.get_last()));
        jit->get_builder().CreateCondBr(loop_condition.get_result(0), for_body, for_end);
        jit->get_builder().SetInsertPoint(for_inc);
        increment_idx(jit, llvm::cast<llvm::AllocaInst>(loop_idx.get_result(0)));
        jit->get_builder().CreateBr(for_cond);

        // build the body
        jit->get_builder().SetInsertPoint(for_body);
        // get all of the data
        llvm::LoadInst *data = jit->get_builder().CreateLoad(args.get_result(0));
        llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx.get_result(0));
        // now get just the element we want to process
        std::vector<llvm::Value *> data_idx;
        data_idx.push_back(cur_loop_idx);
        llvm::Value *data_gep = jit->get_builder().CreateInBoundsGEP(data, data_idx);
        llvm::LoadInst *element = jit->get_builder().CreateLoad(data_gep);
        llvm::AllocaInst *element_alloc = jit->get_builder().CreateAlloca(element->getType());
        jit->get_builder().CreateStore(element, element_alloc);
        std::vector<llvm::Value *> inputs;
        inputs.push_back(element_alloc);
        jit->get_builder().CreateBr(for_extern_call);

        jit->get_builder().SetInsertPoint(for_extern_call);
        FuncComp call_res = create_extern_call(jit, *func, inputs);
        jit->get_builder().CreateBr(store_data);

        // store the result
        jit->get_builder().SetInsertPoint(store_data);
        store_extern_result(jit, ret_type, ret_struct.get_result(0), ret_idx.get_result(0), call_res.get_result(0));
        increment_idx(jit, llvm::cast<llvm::AllocaInst>(ret_idx.get_result(0)));
        jit->get_builder().CreateBr(for_inc);

        // return the data
        jit->get_builder().SetInsertPoint(for_end);
        return_data(jit, ret_struct.get_result(0), ret_idx.get_result(0));

        func->verify_wrapper();
    }

};

#endif //MATCHIT_TRANSFORMSTAGE_H
