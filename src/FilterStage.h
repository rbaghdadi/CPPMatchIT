//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_FILTERBLOCK_H
#define MATCHIT_FILTERBLOCK_H

#include <string>
#include "./Stage.h"
#include "./CodegenUtils.h"

template <typename I>
class FilterStage : public Stage {

private:

    // TODO this doesn't allow structs of structs
    std::vector<MType *> input_struct_fields;

    bool (*filter)(I);

//    // assumes at the correct spot already in the builder
//    void codegen_postprocess(llvm::BasicBlock *inc_block, llvm::AllocaInst *alloc_input_data, llvm::AllocaInst *alloc_loop_idx,
//                             llvm::AllocaInst *alloc_ret_idx, llvm::AllocaInst *alloc_ret, llvm::Value *call_res) {
//
//        // save the result of the function call
//        llvm::AllocaInst *call_alloc = jit->get_builder().CreateAlloca(call_res->getType());
//        jit->get_builder().CreateStore(call_res, call_alloc);
//
//        // should we keep or remove this computation result?
//        llvm::BasicBlock *keep_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "keep_block", mfunction->get_extern_wrapper());
//        llvm::LoadInst *loaded_call = jit->get_builder().CreateLoad(call_alloc);
//        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(loaded_call, llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), 0), "discard");
//        LLVMIR::branch_cond(jit, cmp, inc_block, keep_block);
//
//        // in the keep block, we add the returned computation to our final return list
//        jit->get_builder().SetInsertPoint(keep_block);
//
//        // load the current ret idx to figure out where to store the output of the transform extern function
//        llvm::LoadInst *ret_idx = jit->get_builder().CreateLoad(alloc_ret_idx);
//        ret_idx->setAlignment(8);
//
//        // load the return struct
//        llvm::LoadInst *loaded_field = LLVMIR::codegen_load_struct_field(jit, alloc_ret, 0);
//        std::vector<llvm::Value *> gep_element_idx;
//        gep_element_idx.push_back(ret_idx);
//        llvm::Value *gep_element = jit->get_builder().CreateInBoundsGEP(loaded_field, gep_element_idx);
//        // load the actual index into the return data that we want to eventually store data into
//        llvm::LoadInst *address = jit->get_builder().CreateLoad(gep_element);
//
//        // now we have the pointer to the address where we should store the input element
//        // but first, we must allocate space for that
//        // TODO this assumes that the underlying element type is some type of array or pointer, which it might not always be, so we need some type of check
//        llvm::Value *bitcast = LLVMIR::codegen_c_malloc_and_cast(jit, (mfunction->get_extern_wrapper_data_ret_type()->get_bits() / 8) * 50,
//                                                                 mfunction->get_extern_wrapper_data_ret_type()->codegen());
//
//        // load the return struct again
//        loaded_field = LLVMIR::codegen_load_struct_field(jit, alloc_ret, 0);
//
//        // store the malloc'd space in the return struct
//        LLVMIR::codegen_store_malloc_in_struct(jit, loaded_field, bitcast, alloc_ret_idx);
//
//        // now we get that same address again :)
//        ret_idx = jit->get_builder().CreateLoad(alloc_ret_idx);
//        ret_idx->setAlignment(8);
//        loaded_field = LLVMIR::codegen_load_struct_field(jit, alloc_ret, 0);
//        gep_element = jit->get_builder().CreateInBoundsGEP(loaded_field, gep_element_idx);
//        address = jit->get_builder().CreateLoad(gep_element);
//
//        // copy over the input data. first get the data, then get the correct index of it
//        llvm::LoadInst *loaded_data = jit->get_builder().CreateLoad(alloc_input_data);
//        llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(alloc_loop_idx);
//        std::vector<llvm::Value *> gep_idxs;
//        gep_idxs.push_back(cur_loop_idx);
//        llvm::Value *gep = jit->get_builder().CreateInBoundsGEP(loaded_data, gep_idxs);
//        llvm::LoadInst *loaded_element = jit->get_builder().CreateLoad(gep);
//        LLVMIR::codegen_llvm_memcpy(jit, address, loaded_element);
//
//        // increment the return index
//        LLVMIR::codegen_ret_idx_inc(jit, alloc_ret_idx);
//    }

public:

    FilterStage(bool (*filter)(I), std::string filter_name, JIT *jit, std::vector<MType *> input_struct_fields = std::vector<MType *>()) :
            Stage(jit, mtype_of<I>(), mtype_bool, filter_name), input_struct_fields(input_struct_fields), filter(filter) {
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
        MFunc *func = new MFunc(function_name, "FilterStage", create_type<bool>(), arg_types, jit);
        set_function(func);
        func->codegen_extern_proto();

    }

    ~FilterStage() {}

    void codegen() {
        mfunction->codegen_extern_wrapper_proto();
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

        // determine if this input should be kept or not
        dummy_block->insertInto(mfunction->get_extern_wrapper());
        jit->get_builder().CreateBr(dummy_block);
        postprocess(this, dummy_block, extern_call_store_basic_block->get_basic_block());
//        llvm::BasicBlock *postprocess = llvm::BasicBlock::Create(llvm::getGlobalContext(), "postprocess", mfunction->get_extern_wrapper());
//        jit->get_builder().CreateBr(postprocess);
//        jit->get_builder().SetInsertPoint(postprocess);
//        llvm::LoadInst *load = jit->get_builder().CreateLoad(extern_call_basic_block->get_data_to_return());
//        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(load, llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), 0));
//        extern_call_basic_block->override_data_to_return(extern_init_basic_block->get_element());
//        jit->get_builder().CreateCondBr(cmp, for_loop_increment_basic_block->get_basic_block(), extern_call_store_basic_block->get_basic_block());

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
        jit->get_builder().SetInsertPoint(branch_from);
        llvm::BasicBlock *postprocess = llvm::BasicBlock::Create(llvm::getGlobalContext(), "postprocess", stage->get_mfunction()->get_extern_wrapper());
        jit->get_builder().CreateBr(postprocess);
        jit->get_builder().SetInsertPoint(postprocess);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(stage->get_extern_call_basic_block()->get_data_to_return());
        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(load, llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), 0));
        stage->get_extern_call_basic_block()->override_data_to_return(stage->get_extern_init_basic_block()->get_element());
        jit->get_builder().CreateCondBr(cmp, stage->get_for_loop_increment_basic_block()->get_basic_block(), branch_into);//stage->get_dummy_block());//extern_call_store_basic_block->get_basic_block());
//        jit->get_builder().SetInsertPoint(branch_from);//stage->get_dummy_block());
//        jit->get_builder().CreateBr(stage->get_extern_call_store_basic_block()->get_basic_block());//extern_call_store_basic_block->get_basic_block());
    }

//    void codegen() {
//        MType *arg_type;
//        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
//        if(input_type_code == mtype_ptr && !input_struct_fields.empty()) {
//            arg_type = create_struct_reference_type(input_struct_fields);
//        } else if (input_type_code != mtype_struct) {
//            arg_type = create_type<I>();
//        } else {
//            arg_type = create_struct_type(input_struct_fields);
//        }
//        std::vector<MType *> arg_types;
//        arg_types.push_back(arg_type);
//        MFunc *func = new MFunc(function_name, "FilterStage", create_type<bool>(), arg_types, jit);
//        set_function(func);
//
//        // build up the boilerplate parts of the block
//        generic_codegen_tuple tup = codegen_init();
//
//        // ignore error in clion
//        // add on the Block specific processing
//        codegen_postprocess(std::get<0>(tup), std::get<6>(tup), std::get<2>(tup), std::get<3>(tup), std::get<4>(tup), std::get<5>(tup));
//        LLVMIR::branch(jit, std::get<0>(tup));
//
//        // create the return after the loop
//        codegen_loop_end_block(std::get<1>(tup), std::get<4>(tup), std::get<3>(tup));
//
//        func->verify_wrapper();
//    }

};

#endif //MATCHIT_FILTERBLOCK_H
