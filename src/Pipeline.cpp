//
// Created by Jessica Ray on 1/28/16.
//

#include <stdio.h>
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "./JIT.h"
#include "./Pipeline.h"
#include "./CodegenUtils.h"

using namespace CodegenUtils;

void Pipeline::register_stage(Stage *stage) {
    stages.push_back(stage);
}

void Pipeline::simple_execute(JIT *jit, const void **data) {
    jit->run("pipeline", data);
}

void Pipeline::register_stage(Stage *stage, Relation *input_relation, Relation *output_relation) {
    std::tuple<Stage *, Relation *, Relation *> parameterized_stage(stage, input_relation, output_relation);
    paramaterized_stages.push_back(parameterized_stage);
}

void Pipeline::register_stage(Stage *stage, Relation *input_relation) {
    std::tuple<Stage *, Relation *, Relation *> parameterized_stage(stage, input_relation, nullptr);
    paramaterized_stages.push_back(parameterized_stage);
}

void Pipeline::codegen(JIT *jit) {
    assert(!paramaterized_stages.empty());

    std::tuple<Stage *, Relation *, Relation *> cur_parameterized_stage = paramaterized_stages[0];
    // ignore type error in Clion
    Stage *stage = std::get<0>(cur_parameterized_stage);
    Relation *input_relation = std::get<1>(cur_parameterized_stage);
    Relation *output_relation = std::get<2>(cur_parameterized_stage);

    stage->codegen();
    MFunc *mfunction = stage->get_mfunction();
    llvm::Function *llvm_stage = mfunction->get_extern_wrapper();

    // The pipeline is a function that wraps all the stages and passes data between them.
    // It takes as inputs the inputs to the first stage
    std::vector<llvm::Type *> pipeline_args;
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[0]->codegen_type()); // input SetElements
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[1]->codegen_type()); // number of input SetElements
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[2]->codegen_type()); // output SetElements
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[3]->codegen_type()); // number of output SetElements
    if (!stage->is_filter()) {
        std::vector<BaseField *> output_relation_fields = output_relation->get_fields();
        for (std::vector<BaseField *>::iterator field = output_relation_fields.begin();
             field != output_relation_fields.end(); field++) {
            MType *field_type = new MPointerType(create_field_type((*field)->get_data_mtype()));
            pipeline_args.push_back(codegen_llvm_ptr(jit, field_type->codegen_type()));//to_data_mtype()->codegen_type()));
        }
    }
    // DEBUG
    MType *field_type = new MPointerType(create_debug_type());
    pipeline_args.push_back(codegen_llvm_ptr(jit, field_type->codegen_type()));
    // END DEBUG

    // Make the llvm pipeline function
    // the pipeline doesn't return anything
    llvm::FunctionType *pipeline_function_type = llvm::FunctionType::get(llvm_void, pipeline_args, false);
    llvm::Function *pipeline = llvm::Function::Create(pipeline_function_type, llvm::Function::ExternalLinkage,
                                                      "pipeline", jit->get_module().get());
    llvm::BasicBlock *pipeline_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", pipeline);
    jit->get_builder().SetInsertPoint(pipeline_bb);

    // prep inputs to the first stage
    std::vector<llvm::Value *> llvm_stage_args;
    int iter_ctr = 0;
    for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
        // allocate space and load the arguments
        if (iter_ctr < 4) { // these are the set elements arguments
            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit,
                                                          mfunction->get_extern_wrapper_param_types()[iter_ctr]->codegen_type(),
                                                          8);
            codegen_llvm_store(jit, iter, alloc, 8);
            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
            llvm_stage_args.push_back(load);
        } else if (iter_ctr < 6) { // these are the field args
            // the ** type
            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit,
                                                          codegen_llvm_ptr(jit, mfunction->get_extern_wrapper_param_types()[iter_ctr]->codegen_type()),
                                                          8);
            codegen_llvm_store(jit, iter, alloc, 8);
            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
            // get the * from the ** because these are only single field vectors
            std::vector<llvm::Value *> gep_idx;
            gep_idx.push_back(get_i32(0));
            llvm::LoadInst *final_load = codegen_llvm_load(jit, codegen_llvm_gep(jit, load, gep_idx), 8);
            llvm_stage_args.push_back(final_load);

            if (iter_ctr == 5)
            {
                gep_idx.push_back(get_i32(6));
                llvm::LoadInst *final_load2 = codegen_llvm_load(jit, codegen_llvm_gep(jit, final_load, gep_idx), 8);
                codegen_fprintf_int(jit, 444);
                codegen_fprintf_int(jit, final_load2);
            }


        } else if (iter_ctr == 6) { // DEBUG
            // the ** type
            MType *blah = create_debug_type();
            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit,
                                                          codegen_llvm_ptr(jit, codegen_llvm_ptr(jit, blah->codegen_type())),
                                                          8);
            codegen_llvm_store(jit, iter, alloc, 8);
            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
            // get the * from the ** because these are only single field vectors
            std::vector<llvm::Value *> gep_idx;
            gep_idx.push_back(get_i64(0));
            llvm::LoadInst *final_load = codegen_llvm_load(jit, codegen_llvm_gep(jit, load, gep_idx), 8);
            gep_idx.push_back(get_i32(2));
            llvm::LoadInst *final_load2 = codegen_llvm_load(jit, codegen_llvm_gep(jit, final_load, gep_idx), 8);
            codegen_fprintf_int(jit, 555);
            codegen_fprintf_int(jit, final_load2);
        } // END DEBUG

        // okay, it is reading in the input SetElements okay
        // okay, it is reading in the output SetElements okay
//        if (iter_ctr == 2) {
//            std::vector<llvm::Value *> tmp;
//            tmp.push_back(get_i64(2));
//            llvm::Value *src = codegen_llvm_load(jit, alloc, 8);
//            llvm::Value *tmpgep = codegen_llvm_load(jit, codegen_llvm_gep(jit, src, tmp), 8);
//            std::vector<llvm::Value *> tmp2;
//            tmp2.push_back(get_i32(0));
//            tmp2.push_back(get_i32(0));
//
////            llvm::Value *src2 = codegen_llvm_load(jit, tmpgep, 8);
////            jit->dump();
//            llvm::Value *tmpgep2 = codegen_llvm_load(jit, codegen_llvm_gep(jit, tmpgep, tmp2), 8);
//            codegen_fprintf_int(jit, 98765);
//            codegen_fprintf_int(jit, tmpgep2);
//        }
        iter_ctr++;
    }
    // create the stage calls and chain them together
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *call = jit->get_builder().CreateCall(llvm_stage, llvm_stage_args);
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    jit->get_builder().CreateRet(nullptr); // ret void
    mfunction->verify_wrapper();
}

void Pipeline::codegen_old(JIT *jit, size_t num_prim_values, size_t num_structs) {

    assert(!stages.empty());

    /*
     * Create the pipeline wrapper
     */

    stages[0]->codegen();
    MFunc *mfunction = stages[0]->get_mfunction();
    llvm::Function *llvm_extern_wrapper = mfunction->get_extern_wrapper();

    // the argument types of the llvm function that we want to call
    std::vector<llvm::Type *> pipeline_arg_types;
    pipeline_arg_types.push_back(llvm::PointerType::get(mfunction->get_extern_param_types()[0]->codegen_type(), 0));

    // our pipeline wrapper function
    // add the return type
    llvm::FunctionType *llvm_func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()),
                                                                 pipeline_arg_types, false);
    // create the function
    llvm::Function *wrapper = llvm::Function::Create(llvm_func_type, llvm::Function::ExternalLinkage, "pipeline", jit->get_module().get());
    llvm::BasicBlock *wrapper_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry_block", wrapper);
    jit->get_builder().SetInsertPoint(wrapper_block);

    // give names to the inputs of our wrapper function
    int iter_ctr = 0;
    for (llvm::Function::arg_iterator iter = wrapper->arg_begin(); iter != wrapper->arg_end(); iter++) {
        llvm::Value *arg = iter;
        arg->setName("pipe_x" + std::to_string(iter_ctr++));
    }

    /*
     * Prep the input data that will be fed into the first block
     */

    std::vector<llvm::Value *> llvm_func_args;
    iter_ctr = 0;
    for (llvm::Function::arg_iterator iter = wrapper->arg_begin(); iter != wrapper->arg_end(); iter++) {
        // allocate space and load the arguments
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(mfunction->get_extern_wrapper_param_types()[iter_ctr]->codegen_type());
        alloc->setAlignment(8);
        jit->get_builder().CreateStore(iter, alloc)->setAlignment(8);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(alloc);
        load->setAlignment(8);
        llvm_func_args.push_back(load);
        iter_ctr++;
    }
    llvm_func_args.push_back(CodegenUtils::get_i64(num_prim_values));
    llvm_func_args.push_back(CodegenUtils::get_i64(num_structs));

    /*
     * Create the stage calls and chain them together
     */

    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *call = jit->get_builder().CreateCall(llvm_extern_wrapper, llvm_func_args);

    // print a separator to make the output easier to parse
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    Stage *prev_block = stages[0];
    for (std::vector<Stage *>::iterator iter = stages.begin() + 1; iter != stages.end(); iter++) {
        (*iter)->codegen();
        jit->get_builder().SetInsertPoint(wrapper_block);
        // split out the fields of the returned struct
//        llvm::LoadInst *field_one = gep_i64_i32_and_load(jit, call, 0, 0);
//        llvm::LoadInst *field_two = gep_i64_i32_and_load(jit, call, 0, 1);
//        llvm::LoadInst *field_three = gep_i64_i32_and_load(jit, call, 0, 2);
//        std::vector<llvm::Value *> args;
//        args.push_back(field_one);
//        args.push_back(field_two);
//        args.push_back(field_three);
//        call = jit->get_builder().CreateCall((*iter)->get_mfunction()->get_extern_wrapper(), args);
        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
        prev_block = (*iter);
    }

    jit->get_builder().CreateRet(nullptr);
    mfunction->verify_wrapper();
}