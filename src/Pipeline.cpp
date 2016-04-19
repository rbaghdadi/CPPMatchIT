//
// Created by Jessica Ray on 1/28/16.
//

#include <stdio.h>
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "./CodegenUtils.h"
#include "./Field.h"
#include "./JIT.h"
#include "./Pipeline.h"
#include "SegmentationStage.h"

using namespace Codegen;

//void Pipeline::simple_execute(JIT *jit, const void **data) {
//    jit->run("pipeline", data);
//}

void Pipeline::register_stage(Stage *stage) {
    stages.push_back(stage);
}

void Pipeline::codegen(JIT *jit) {
    assert(!stages.empty());

    // codegen the stages ahead of time
    for (std::vector<Stage *>::iterator iter = stages.begin(); iter != stages.end(); iter++) {
        (*iter)->codegen();
    }

    Stage *stage = stages[0];
    std::vector<BaseField *> output_relation_fields = stage->get_output_relation_field_types();
    unsigned long num_output_relation_fields = output_relation_fields.size();

    MFunc *mfunction = stage->get_mfunction();
    llvm::Function *llvm_stage = mfunction->get_extern_wrapper();

    // The pipeline is a function that wraps all the stages and passes data between them.
    // It takes as inputs the inputs to the first stage
    std::vector<llvm::Type *> pipeline_args;
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[0]->codegen_type()); // input SetElements
    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[1]->codegen_type()); // number of input SetElements
    // find out the number of output fields across all the stages (inputs are just the previous stages outputs) and then add them to the signature
    int total_num_output_fields = 0;
    for (std::vector<Stage *>::iterator iter = stages.begin(); iter != stages.end(); iter++) {
        unsigned long num_output_relation_fields = (*iter)->get_output_relation_field_types().size();
        total_num_output_fields += num_output_relation_fields;
    }

    for (int i = 0; i < total_num_output_fields; i++) {
        MType *field_type = new MPointerType(create_type<BaseField>());
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
    std::vector<llvm::Value *> tmp_comparison_args; // only needed for comparison--used to duplicate the input
    llvm::Value *inputs_to_next_stage;
    MType *type = create_type<SetElement>();
    llvm::Type *setelement_type = llvm::PointerType::get(llvm::PointerType::get(type->codegen_type(), 0), 0);
//    int iter_ctr = 0;
    // get the fields from the pipeline parameters
    for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
        llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
        codegen_llvm_store(jit, iter, alloc, 8);
        llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
        // if the type is not {i32}** or i32, then it is a field
        if (iter->getType() != setelement_type && iter->getType() != llvm_int32) {
            fields.push_back(load);
        }

    }

    // get the arguments that just belong to the first stage
    if (stage->is_filter()) {
        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
            codegen_llvm_store(jit, iter, alloc, 8);
            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
            if (iter->getType() == setelement_type || iter->getType() == llvm_int32) {
                llvm_stage_args.push_back(load);
            } else {
                break;
            }
        }
        inputs_to_next_stage = llvm_stage_args[0]; // FilterStage has no outputs, it just passes through an input if it's kept
    } else if (stage->is_segmentation()) {
        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
            codegen_llvm_store(jit, iter, alloc, 8);
            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
            if (iter->getType() == setelement_type || iter->getType() == llvm_int32) {
                llvm_stage_args.push_back(load);
            } else {
                break;
            }
        }
        // create output SetElements
        std::vector<llvm::Value *> create_setelements_args;
        llvm::LoadInst *num_of_output_setelements = codegen_llvm_load(jit, ((SegmentationStage *) stage)->compute_num_output_structs(), 4);
        create_setelements_args.push_back(num_of_output_setelements);
        llvm::Value *setelements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_setelements"),
                                                                 create_setelements_args);
        inputs_to_next_stage = setelements;
        llvm_stage_args.push_back(setelements);
        llvm_stage_args.push_back(num_of_output_setelements);
    } else if (stage->is_comparison()) {
        int iter_ctr = 0;
        std::vector<llvm::Value *> duplicates;
        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
            codegen_llvm_store(jit, iter, alloc, 8);
            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
            if (iter->getType() == setelement_type || iter->getType() == llvm_int32) {
                if (iter_ctr < 2) {
                    duplicates.push_back(load); // this is either the input SetElements or the number of input SetElements. Either way, for comparison, it needs to be replicated.
                }
                llvm_stage_args.push_back(load);
            } else {
                // duplicate the inputs
                for (std::vector<llvm::Value *>::iterator dup = duplicates.begin(); dup != duplicates.end(); dup++) {
                    llvm_stage_args.push_back(*dup);
                }
                break;
            }
            iter_ctr++;
        }
        // if there are outputs in this ComparisonStage, create the SetElements for them
        if (!output_relation_fields.empty()) {
            std::vector<llvm::Value *> create_setelements_args;
            llvm::Value *num_of_output_setelements = llvm_stage_args[1];
            create_setelements_args.push_back(codegen_llvm_mul(jit, num_of_output_setelements, num_of_output_setelements));
            llvm::Value *setelements = jit->get_builder().CreateCall(
                    jit->get_module()->getFunction("create_setelements"),
                    create_setelements_args);
            inputs_to_next_stage = setelements;
            llvm_stage_args.push_back(setelements);
            llvm_stage_args.push_back(num_of_output_setelements);
        } else {
            inputs_to_next_stage = llvm_stage_args[0];
        }
    } else {
        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
            codegen_llvm_store(jit, iter, alloc, 8);
            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
            if (iter->getType() == setelement_type || iter->getType() == llvm_int32) {
                llvm_stage_args.push_back(load);
            } else {
                break;
            }
        }
        std::vector<llvm::Value *> create_setelements_args;
        llvm::Value *num_of_output_setelements = llvm_stage_args[1];
        create_setelements_args.push_back(num_of_output_setelements);
        std::cerr << "dumping" << std::endl;
        jit->get_module()->getFunction("create_setelements")->dump();
        llvm::Value *setelements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_setelements"),
                                                                 create_setelements_args);
        inputs_to_next_stage = setelements;
        llvm_stage_args.push_back(setelements);
        llvm_stage_args.push_back(num_of_output_setelements);
    }

//    jit->dump();

    int field_idx = 0;
    // TODO add the fields
    for (int i = 0; i < output_relation_fields.size(); i++) {
        llvm_stage_args.push_back(fields[field_idx++]);
    }

    // call the first stage
    std::cerr << stage->get_mfunction()->get_extern_wrapper_name() << std::endl;
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *num_outputs = jit->get_builder().CreateCall(llvm_stage, llvm_stage_args);
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    // chain the other stages together
    for (std::vector<Stage *>::iterator iter = stages.begin() + 1; iter != stages.end(); iter++) {
        stage = *iter;
        output_relation_fields = stage->get_output_relation_field_types();
        num_output_relation_fields = output_relation_fields.size();
        mfunction = stage->get_mfunction();
        llvm_stage = mfunction->get_extern_wrapper();
        std::vector<llvm::Value *> llvm_stage_args;
        // use the output SetElements from the previous stage as inputs
        llvm_stage_args.push_back(inputs_to_next_stage); // pass the previous outputs into this stage as inputs
        llvm_stage_args.push_back(num_outputs);
        // make the new output setelements
        // TODO modify here to allow outputs for comparison stage
        if (stage->is_comparison()) { // duplicate the input setelements
            llvm_stage_args.push_back(inputs_to_next_stage);
            llvm_stage_args.push_back(num_outputs);
        } else if (!stage->is_filter()) { // FilterStage only gets input SetElements passed in. It returns the inputs that weren't filtered out
            std::vector<llvm::Value *> setelement_args;
            if (!stage->is_segmentation()) {
                setelement_args.push_back(num_outputs);
            } else {
                num_outputs = ((SegmentationStage *) stage)->compute_num_segments(num_outputs);
                setelement_args.push_back(num_outputs);
            }
            llvm::Value *setelements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_setelements"),
                                                                     setelement_args);
            inputs_to_next_stage = setelements;
            llvm_stage_args.push_back(setelements);
            llvm_stage_args.push_back(num_outputs);
            // the fields for this stage
            if (num_output_relation_fields != 0) { // some stages (like Filter) don't have an output relation because the input relation is passed along instead
                for (int i = field_idx; i < field_idx + num_output_relation_fields; i++) {
                    llvm_stage_args.push_back(fields[i]);
                }
                field_idx += num_output_relation_fields;
            }
        }
        // call the next stage
        num_outputs = jit->get_builder().CreateCall(llvm_stage, llvm_stage_args);
        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    }


    jit->get_builder().CreateRet(nullptr);
    mfunction->verify_wrapper();
}


////
//// Created by Jessica Ray on 1/28/16.
////
//
//#include <stdio.h>
//#include "llvm/IR/Function.h"
//#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/Type.h"
//#include "llvm/IR/Verifier.h"
//#include "./CodegenUtils.h"
//#include "./Field.h"
//#include "./JIT.h"
//#include "./Pipeline.h"
//#include "SegmentationStage.h"
//
//using namespace Codegen;
//
//void Pipeline::simple_execute(JIT *jit, const void **data) {
//    jit->run("pipeline", data);
//}
//
//void Pipeline::register_stage(Stage *stage) {
//    stages.push_back(stage);
//}
//
//void Pipeline::codegen(JIT *jit) {
//    assert(!stages.empty());
//
//    // codegen the stages ahead of time
//    for (std::vector<Stage *>::iterator iter = stages.begin(); iter != stages.end(); iter++) {
//        (*iter)->codegen();
//    }
//
//    Stage *stage = stages[0];
//    std::vector<BaseField *> output_relation_fields = stage->get_output_relation_field_types();
//    unsigned long num_output_relation_fields = output_relation_fields.size();
//
//    MFunc *mfunction = stage->get_mfunction();
//    llvm::Function *llvm_stage = mfunction->get_extern_wrapper();
//
//    // The pipeline is a function that wraps all the stages and passes data between them.
//    // It takes as inputs the inputs to the first stage
//    std::vector<llvm::Type *> pipeline_args;
//    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[0]->codegen_type()); // input SetElements
//    pipeline_args.push_back(mfunction->get_extern_wrapper_param_types()[1]->codegen_type()); // number of input SetElements
//    // find out the number of output fields across all the stages (inputs are just the previous stages outputs) and then add them to the signature
//    int total_num_output_fields = 0;
//    for (std::vector<Stage *>::iterator iter = stages.begin(); iter != stages.end(); iter++) {
//        unsigned long num_output_relation_fields = (*iter)->get_output_relation_field_types().size();
//        total_num_output_fields += num_output_relation_fields;
//    }
//
//    for (int i = 0; i < total_num_output_fields; i++) {
//        MType *field_type = new MPointerType(create_type<BaseField>());
//        pipeline_args.push_back(field_type->codegen_type());
//    }
//
//    // Make the llvm pipeline function
//    // the pipeline doesn't return anything
//    llvm::FunctionType *pipeline_function_type = llvm::FunctionType::get(llvm_void, pipeline_args, false);
//    llvm::Function *pipeline = llvm::Function::Create(pipeline_function_type, llvm::Function::ExternalLinkage,
//                                                      "pipeline", jit->get_module().get());
//    llvm::BasicBlock *pipeline_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", pipeline);
//    jit->get_builder().SetInsertPoint(pipeline_bb);
//
//    // load all the pipeline inputs and get the inputs for the first stage
//    std::vector<llvm::Value *> llvm_stage_args;
//    std::vector<llvm::Value *> fields;
//    std::vector<llvm::Value *> tmp_comparison_args; // only needed for comparison--used to duplicate the input
//    llvm::Value *inputs_to_next_stage;
////
//    int iter_ctr = 0;
//    for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
//        // allocate space and load the arguments
//        llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
//        codegen_llvm_store(jit, iter, alloc, 8);
//        llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
//        if (iter_ctr == 0) {
//            inputs_to_next_stage = load; // will be overwritten later if it's not a filter stage
//        }
//        if (stage->is_filter() && iter_ctr < 2) {
//            llvm_stage_args.push_back(load);
//        } else if (!stage->is_filter() && iter_ctr < 2 + num_output_relation_fields) { // only store the fields for this first stage as inputs (filter has no fields)
//            llvm_stage_args.push_back(load);
//            if (iter_ctr < 2) {
//                tmp_comparison_args.push_back(load);
//            } else if (iter_ctr == 2 && stage->is_comparison()) { // duplicate the inputs
//                llvm_stage_args.push_back(tmp_comparison_args[0]);
//                llvm_stage_args.push_back(tmp_comparison_args[1]);
//            }
//        }
//        if (!stage->is_filter()) { // FilterStage only gets input SetElements passed in. It returns the inputs
//            if (iter_ctr == 1) { // just added the number of input SetElements. Create the corresponding output SetElements and tack them on
//                std::vector<llvm::Value *> setelement_args;
//                if (!stage->is_segmentation()) {
//                    setelement_args.push_back(load); // the loop bound is the number of elements
//                } else {
//                    load = codegen_llvm_load(jit, ((SegmentationStage *) stage)->compute_num_output_structs(), 4);
//                    setelement_args.push_back(load);
//                }
//                llvm::Value *setelements = jit->get_builder().CreateCall(
//                        jit->get_module()->getFunction("create_setelements"),
//                        setelement_args);
//                inputs_to_next_stage = setelements;
//                llvm_stage_args.push_back(setelements);
//                llvm_stage_args.push_back(load); // the number of output setelements
//            }
//        }
//        if (iter_ctr > 1) { // these are all of the fields of the output
//            fields.push_back(load);
//        }
//        iter_ctr++;
//    }
//    int field_idx;
//    if (num_output_relation_fields != 0) {
//        field_idx = num_output_relation_fields;
//    } else {
//        field_idx = 0;
//    }
//
//    // call the first stage
//    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
//    llvm::Value *num_outputs = jit->get_builder().CreateCall(llvm_stage, llvm_stage_args);
//    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
//
//    // chain the other stages together
//    for (std::vector<Stage *>::iterator iter = stages.begin() + 1; iter != stages.end(); iter++) {
//        stage = *iter;
//        output_relation_fields = stage->get_output_relation_field_types();
//        num_output_relation_fields = output_relation_fields.size();
//        mfunction = stage->get_mfunction();
//        llvm_stage = mfunction->get_extern_wrapper();
//        std::vector<llvm::Value *> llvm_stage_args;
//        // use the output SetElements from the previous stage as inputs
//        std::vector<llvm::Value *> call_idxs;
//        call_idxs.push_back(as_i64(0));
//        call_idxs.push_back(as_i32(0)); // get the SetElements
//        llvm_stage_args.push_back(inputs_to_next_stage); // pass the previous outputs into this stage as inputs
//        call_idxs.clear();
//        call_idxs.push_back(as_i64(0));
//        call_idxs.push_back(as_i32(1)); // get the number of SetElements
//        llvm_stage_args.push_back(num_outputs);
//        // make the new output setelements
//        // TODO modify here to allow outputs for comparison stage
//        if (stage->is_comparison()) { // duplicate the input setelements
//            llvm_stage_args.push_back(inputs_to_next_stage);
//            llvm_stage_args.push_back(num_outputs);
//        } else if (!stage->is_filter()) { // FilterStage only gets input SetElements passed in. It returns the inputs that weren't filtered out
//            std::vector<llvm::Value *> setelement_args;
//            if (!stage->is_segmentation()) {
//                setelement_args.push_back(num_outputs);
//            } else {
//                num_outputs = ((SegmentationStage *) stage)->compute_num_segments(num_outputs);
//                codegen_fprintf_int(jit, num_outputs);
//                setelement_args.push_back(num_outputs);
//            }
//            llvm::Value *setelements = jit->get_builder().CreateCall(
//                    jit->get_module()->getFunction("create_setelements"),
//                    setelement_args);
//            inputs_to_next_stage = setelements;
//            llvm_stage_args.push_back(setelements);
//            llvm_stage_args.push_back(num_outputs);
//            // the fields for this stage
//            if (num_output_relation_fields != 0) { // some stages (like Filter) don't have an output relation because the input relation is passed along instead
//                for (int i = field_idx; i < field_idx + num_output_relation_fields; i++) {
//                    llvm_stage_args.push_back(fields[i]);
//                }
//                field_idx += num_output_relation_fields;
//            }
//        }
//        // call the next stage
//        num_outputs = jit->get_builder().CreateCall(llvm_stage, llvm_stage_args);
//        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
//    }
//
//
//    jit->get_builder().CreateRet(nullptr);
//    mfunction->verify_wrapper();
//}
