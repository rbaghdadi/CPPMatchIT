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
#include "./Field.h"

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

    // codegen everything ahead of time
    for (std::vector<std::tuple<Stage *, Relation *, Relation *>>::iterator iter = paramaterized_stages.begin(); iter != paramaterized_stages.end(); iter++) {
        Stage *stage = std::get<0>(*iter);
        stage->codegen();
    }

    std::tuple<Stage *, Relation *, Relation *> first_stage = paramaterized_stages[0];
    Stage *stage = std::get<0>(first_stage);
    Relation *output_relation = std::get<2>(first_stage);

    MFunc *mfunction = stage->get_mfunction();
    llvm::Function *llvm_stage = mfunction->get_extern_wrapper();

    // The pipeline is a function that wraps all the stages and passes data between them.
    // It takes as inputs the inputs to the first stage
    std::vector<llvm::Type *> pipeline_args;
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[0]->codegen_type()); // input SetElements
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[1]->codegen_type()); // number of input SetElements
    // find out the number of output fields across all the stages (inputs are just the previous stages outputs) and then add them to the signature
    int num_output_fields = 0;
    for (std::vector<std::tuple<Stage *, Relation *, Relation *>>::iterator iter = paramaterized_stages.begin();
         iter != paramaterized_stages.end(); iter++) {
        Relation *output_relation = std::get<2>(first_stage);
        num_output_fields += output_relation->get_fields().size();
    }

    for (int i = 0; i < num_output_fields; i++) {
        MType *field_type = new MPointerType(create_field_type());
        pipeline_args.push_back(field_type->codegen_type());
    }

    // Make the llvm pipeline function
    // the pipeline doesn't return anything
    llvm::FunctionType *pipeline_function_type = llvm::FunctionType::get(llvm_void, pipeline_args, false);
    llvm::Function *pipeline = llvm::Function::Create(pipeline_function_type, llvm::Function::ExternalLinkage,
                                                      "pipeline", jit->get_module().get());
    llvm::BasicBlock *pipeline_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", pipeline);
    jit->get_builder().SetInsertPoint(pipeline_bb);

    // load all the pipeline inputs and get the inputs for the first stage
    std::vector<llvm::Value *> llvm_stage_args;
    std::vector<llvm::Value *> fields;
    int iter_ctr = 0;
    for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
        // allocate space and load the arguments
        llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
        codegen_llvm_store(jit, iter, alloc, 8);
        llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
        if (iter_ctr < 2 + output_relation->get_fields().size()) { // only store the fields for this first stage as inputs
            llvm_stage_args.push_back(load);
        }
        if (iter_ctr == 1) { // just added the number of input SetElements. Create the corresponding output SetElements and tack them on
            std::vector<llvm::Value *> setelement_args;
            setelement_args.push_back(load);
            llvm::Value *setelements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_setelements"),
                                                        setelement_args);
            llvm_stage_args.push_back(setelements);
            llvm_stage_args.push_back(load); // the number of output setelements
        }
        if (iter_ctr > 1) { // these are all of the fields
            fields.push_back(load);
        }
        iter_ctr++;
    }
    int field_idx = output_relation->get_fields().size();
    // call the first stage
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *call = jit->get_builder().CreateCall(llvm_stage, llvm_stage_args);
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    // chain the other stages together
    for (std::vector<std::tuple<Stage *, Relation *, Relation *>>::iterator iter = paramaterized_stages.begin() + 1; iter != paramaterized_stages.end(); iter++) {
        stage = std::get<0>(*iter);
        output_relation = std::get<2>(*iter);
        mfunction = stage->get_mfunction();
        llvm_stage = mfunction->get_extern_wrapper();
        std::vector<llvm::Value *> llvm_stage_args;
        // use the output SetElements from the previous stage as inputs
        std::vector<llvm::Value *> call_idxs;
        call_idxs.push_back(get_i64(0));
        call_idxs.push_back(get_i32(0)); // get the SetElements
        llvm::Value *inputs_setelements = codegen_llvm_load(jit, codegen_llvm_gep(jit, call, call_idxs), 8);
        llvm_stage_args.push_back(inputs_setelements);
        call_idxs.clear();
        call_idxs.push_back(get_i64(0));
        call_idxs.push_back(get_i32(1)); // get the number of SetElements
        llvm::Value *num = codegen_llvm_load(jit, codegen_llvm_gep(jit, call, call_idxs), 8);
        llvm_stage_args.push_back(num);
        // make the new output setelements
        std::vector<llvm::Value *> setelement_args;
        setelement_args.push_back(num);
        llvm::Value *setelements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_setelements"),
                                                    setelement_args);
        llvm_stage_args.push_back(setelements);
        llvm_stage_args.push_back(num);
        // the fields for this stage
        for (int i = field_idx; i < field_idx + output_relation->get_fields().size(); i++) {
            llvm_stage_args.push_back(fields[i]);
        }
        field_idx += output_relation->get_fields().size();
        // call things
        call = jit->get_builder().CreateCall(llvm_stage, llvm_stage_args);
        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    }


    jit->get_builder().CreateRet(nullptr);
    mfunction->verify_wrapper();
}
