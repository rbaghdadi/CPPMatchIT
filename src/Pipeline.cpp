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
#include "./SegmentationStage.h"

using namespace Codegen;

void Pipeline::register_stage(Stage *stage) {
    stages.push_back(stage);
}

std::vector<llvm::Value *> get_all_element_params(JIT *jit, llvm::Function *pipeline, llvm::Type *llvm_element_type) {
    std::vector<llvm::Value *> element_params;
    for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
        llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
        codegen_llvm_store(jit, iter, alloc, 8);
        llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
        if (iter->getType() == llvm_element_type || iter->getType() == llvm_int32) {
            element_params.push_back(load);
        } else { // got to the fields. no more Element types after that
            break;
        }
    }
    return element_params;
}

void Pipeline::codegen(JIT *jit) {
    assert(!stages.empty());

    // codegen the llvm code of the individual stages
    for (std::vector<Stage *>::iterator stage = stages.begin(); stage != stages.end(); stage++) {
        (*stage)->codegen();
    }

    Stage *stage = stages[0];
    MFunc *mfunction_stage = stage->get_mfunction();
    std::vector<BaseField *> output_fields = stage->get_output_field_types();
    unsigned long num_stage_output_fields = output_fields.size(); // number of output fields for just this stage.

    /*
     * Build up the prototype for this pipeline function
     */

    // The arguments passed into the pipeline are the fields that are used in the output of the first stage in the pipeline.
    // These fields are created outside of LLVM. However, the first two parameters of the pipeline are the input Elements
    // for the first stage, and the number of input Elements.
    // NOTE: Right now, all fields for the ENTIRE pipeline are passed into the pipeline. So, we read those all here, but
    // later we figure out which fields correspond to which stages.
    // Get the types of the parameters corresponding to those arguments.
    std::vector<llvm::Type *> pipeline_param_types;
    pipeline_param_types.push_back(mfunction_stage->get_stage_param_types()[0]->codegen_type()); // input Elements
    pipeline_param_types.push_back(mfunction_stage->get_stage_param_types()[1]->codegen_type()); // number of input Elements
    int total_num_output_fields = 0; // across all Stages
    for (std::vector<Stage *>::iterator iter = stages.begin(); iter != stages.end(); iter++) {
        unsigned long num_output_fields = (*iter)->get_output_field_types().size();
        total_num_output_fields += num_output_fields;
    }
    // create the appropriate LLVM element_type for the fields.
    for (int i = 0; i < total_num_output_fields; i++) {
        MType *field_type = new MPointerType(create_type<BaseField>()); // all fields passed in as pointers.
        pipeline_param_types.push_back(field_type->codegen_type());
    }
    // actually get the llvm code for the prototype
    llvm::FunctionType *pipeline_function_type = llvm::FunctionType::get(llvm_void, pipeline_param_types, false);
    llvm::Function *pipeline = llvm::Function::Create(pipeline_function_type, llvm::Function::ExternalLinkage,
                                                      "pipeline", jit->get_module().get());
    llvm::BasicBlock *pipeline_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", pipeline);
    jit->get_builder().SetInsertPoint(pipeline_bb);

    /*
     * Read in parameters
     */

    // get all of the fields passed into the pipeline.
    MType *element_type = create_type<Element>();
    llvm::Type *llvm_element_type = llvm::PointerType::get(llvm::PointerType::get(element_type->codegen_type(), 0), 0);
    std::vector<llvm::Value *> fields;
    for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
        llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
        codegen_llvm_store(jit, iter, alloc, 8);
        llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
        // if the element_type is not {i32}** or i32, then it is a field
        if (iter->getType() != llvm_element_type && iter->getType() != llvm_int32) {
            fields.push_back(load);
        }
    }

    // load the pipeline Element parameters that correspond to the first stage of the pipeline
    std::vector<llvm::Value *> llvm_stage_params;
    std::vector<llvm::Value *> tmp_comparison_args; // only needed for comparison--used to duplicate the input
    llvm::Value *inputs_to_next_stage; // inputs to the next stage are output Elements of the first stage (or inputs, in the case of a filter)

    // the type of stage determines the number of Elements passed in
    if (stage->is_filter()) { // no output elements, just input
//        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
//            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
//            codegen_llvm_store(jit, iter, alloc, 8);
//            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
//            if (iter->getType() == llvm_element_type || iter->getType() == llvm_int32) {
//                llvm_stage_params.push_back(load);
//            } else {
//                break;
//            }
//        }
        llvm_stage_params = get_all_element_params(jit, pipeline, llvm_element_type);
        inputs_to_next_stage = llvm_stage_params[0]; // FilterStage has no outputs, it just passes through an input if it's kept
    } else if (stage->is_segmentation()) { // input and outputs
//        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
//            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
//            codegen_llvm_store(jit, iter, alloc, 8);
//            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
//            if (iter->getType() == llvm_element_type || iter->getType() == llvm_int32) {
//                llvm_stage_params.push_back(load);
//            } else {
//                break;
//            }
//        }
        llvm_stage_params = get_all_element_params(jit, pipeline, llvm_element_type);
        // create output Elements
        std::vector<llvm::Value *> create_elements_args;
        llvm::LoadInst *num_elements_to_create = codegen_llvm_load(jit, ((SegmentationStage *) stage)->compute_num_output_elements(), 4);
        create_elements_args.push_back(num_elements_to_create);
        llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"),
                                                                 create_elements_args);
        inputs_to_next_stage = elements;
        llvm_stage_params.push_back(elements);
        llvm_stage_params.push_back(num_elements_to_create);
    } else if (stage->is_comparison()) { // 2 inputs or 2 inputs and an output
        std::vector<llvm::Value *> tmp = get_all_element_params(jit, pipeline, llvm_element_type);
        // for now, the 2 input "streams" to comparison are just duplicates of one another. So, duplicate the input elements
        assert(tmp.size() >= 2);
        llvm_stage_params.push_back(tmp[0]); // Element
        llvm_stage_params.push_back(tmp[1]); // number of Element objects
        llvm_stage_params.push_back(tmp[0]); // duplicate Element
        llvm_stage_params.push_back(tmp[1]); // duplicate number of Element objects
        // check if this stage has an output. If so, create the output Element objects
        if (!output_fields.empty()) {
            std::vector<llvm::Value *> create_elements_args;
            llvm::Value *num_elements_to_create = llvm_stage_params[1];
            create_elements_args.push_back(codegen_llvm_mul(jit, num_elements_to_create, num_elements_to_create));
            llvm::Value *elements = jit->get_builder().CreateCall(
                    jit->get_module()->getFunction("create_elements"),
                    create_elements_args);
            inputs_to_next_stage = elements;
            llvm_stage_params.push_back(elements);
            llvm_stage_params.push_back(num_elements_to_create);
        } else { // no output, just forward the input
            inputs_to_next_stage = llvm_stage_params[0];
        }
//
//        int iter_ctr = 0;
//        std::vector<llvm::Value *> duplicates;
//        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
//            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
//            codegen_llvm_store(jit, iter, alloc, 8);
//            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
//            if (iter->getType() == llvm_element_type || iter->getType() == llvm_int32) {
//                if (iter_ctr < 2) {
//                    duplicates.push_back(load); // this is either the input SetElements or the number of input SetElements. Either way, for comparison, it needs to be replicated.
//                }
//                llvm_stage_params.push_back(load);
//            } else {
//                // duplicate the inputs
//                for (std::vector<llvm::Value *>::iterator dup = duplicates.begin(); dup != duplicates.end(); dup++) {
//                    llvm_stage_params.push_back(*dup);
//                }
//                break;
//            }
//            iter_ctr++;
//        }
//        // if there are outputs in this ComparisonStage, create the SetElements for them
//        if (!output_fields.empty()) {
//            std::vector<llvm::Value *> create_setelements_args;
//            llvm::Value *num_of_output_setelements = llvm_stage_params[1];
//            create_setelements_args.push_back(codegen_llvm_mul(jit, num_of_output_setelements, num_of_output_setelements));
//            llvm::Value *setelements = jit->get_builder().CreateCall(
//                    jit->get_module()->getFunction("create_elements"),
//                    create_setelements_args);
//            inputs_to_next_stage = setelements;
//            llvm_stage_params.push_back(setelements);
//            llvm_stage_params.push_back(num_of_output_setelements);
//        } else {
//            inputs_to_next_stage = llvm_stage_params[0];
//        }
    } else { // some other stage
        llvm_stage_params = get_all_element_params(jit, pipeline, llvm_element_type);
//        for (llvm::Function::arg_iterator iter = pipeline->arg_begin(); iter != pipeline->arg_end(); iter++) {
//            llvm::AllocaInst *alloc = codegen_llvm_alloca(jit, iter->getType(), 8);
//            codegen_llvm_store(jit, iter, alloc, 8);
//            llvm::LoadInst *load = codegen_llvm_load(jit, alloc, 8);
//            if (iter->getType() == llvm_element_type || iter->getType() == llvm_int32) {
//                llvm_stage_params.push_back(load);
//            } else {
//                break;
//            }
//        }
        std::vector<llvm::Value *> create_elements_args;
        llvm::Value *num_elements_to_create = llvm_stage_params[1];
        create_elements_args.push_back(num_elements_to_create);
        llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"),
                                                                 create_elements_args);
        inputs_to_next_stage = elements;
        llvm_stage_params.push_back(elements);
        llvm_stage_params.push_back(num_elements_to_create);
    }

    // add the fields that correspond to the first stage (if necessary)
    int field_idx = 0;
    for (int i = 0; i < output_fields.size(); i++) {
        llvm_stage_params.push_back(fields[field_idx++]);
    }

    // call the first stage
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *num_outputs = jit->get_builder().CreateCall(mfunction_stage->get_llvm_stage(), llvm_stage_params);
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    // chain the other stages together
    for (std::vector<Stage *>::iterator iter = stages.begin() + 1; iter != stages.end(); iter++) {
        llvm_stage_params.clear();
        stage = *iter;
        output_fields = stage->get_output_field_types();
        num_stage_output_fields = output_fields.size();
        mfunction_stage = stage->get_mfunction();
        // the inputs to this current stage are appropriately set in the previous stage
        llvm_stage_params.push_back(inputs_to_next_stage);
        llvm_stage_params.push_back(num_outputs);
        // make the new output elements
        // TODO modify here to allow outputs for comparison stage
        if (stage->is_comparison()) { // duplicate the input setelements
            llvm_stage_params.push_back(inputs_to_next_stage);
            llvm_stage_params.push_back(num_outputs);
            // check if this comparison has an output
//            if (!output_fields.empty()) {
//                std::vector<llvm::Value *> create_elements_args;
//                llvm::Value *num_elements_to_create = llvm_stage_params[1];
//                create_elements_args.push_back(codegen_llvm_mul(jit, num_elements_to_create, num_elements_to_create));
//                llvm::Value *elements = jit->get_builder().CreateCall(
//                        jit->get_module()->getFunction("create_elements"),
//                        create_elements_args);
//                inputs_to_next_stage = elements;
//                llvm_stage_params.push_back(elements);
//                llvm_stage_params.push_back(num_elements_to_create);
//            } // else, the inputs_to_next_stage just stay the same
        }
        if (!stage->is_filter() && !output_fields.empty()) { // filter has no outputs
            std::vector<llvm::Value *> create_element_args;
            if (!stage->is_segmentation()) {
                create_element_args.push_back(num_outputs);
            } else {
                num_outputs = ((SegmentationStage *) stage)->compute_num_segments(num_outputs);
                create_element_args.push_back(num_outputs);
            }
            llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"), create_element_args);
            inputs_to_next_stage = elements;
            llvm_stage_params.push_back(elements);
            llvm_stage_params.push_back(num_outputs);
            for (int i = field_idx; i < field_idx + num_stage_output_fields; i++) {
                llvm_stage_params.push_back(fields[i]);
            }
            field_idx += num_stage_output_fields;
        }
        // create the output Elements and add the fields that correspond to this stage (if necessary)
//        if (!output_fields.empty()) {
//            for (int i = field_idx; i < field_idx + num_stage_output_fields; i++) {
//                llvm_stage_params.push_back(fields[i]);
//            }
//            field_idx += num_stage_output_fields;
//        }
        // call the current stage
        num_outputs = jit->get_builder().CreateCall(mfunction_stage->get_llvm_stage(), llvm_stage_params);
        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    }

    jit->get_builder().CreateRet(nullptr);
    mfunction_stage->verify_wrapper();
}
