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
    if (stage->is_filter()) { // pointers to output elements
        std::vector<llvm::Value *> create_elements_args;
        llvm_stage_params = get_all_element_params(jit, pipeline, llvm_element_type);
        llvm::Value *num_elements_to_create = llvm_stage_params[1];
        create_elements_args.push_back(num_elements_to_create);
        create_elements_args.push_back(as_i1(true)); // don't create new Element objects, just the pointers to them
        llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"),
                                                              create_elements_args);
        llvm_stage_params.push_back(elements);
        llvm_stage_params.push_back(num_elements_to_create);
        inputs_to_next_stage = elements; // FilterStage has no "new" outputs, it just passes through an input if it's kept
    } else if (stage->is_segmentation()) { // input and outputs
        llvm_stage_params = get_all_element_params(jit, pipeline, llvm_element_type);
        // create output Elements
        std::vector<llvm::Value *> create_elements_args;
        llvm::LoadInst *num_elements_to_create = codegen_llvm_load(jit, ((SegmentationStage *) stage)->compute_num_output_elements(), 4);
        create_elements_args.push_back(num_elements_to_create);
        create_elements_args.push_back(as_i1(false));
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
            create_elements_args.push_back(as_i1(false));
            llvm::Value *elements = jit->get_builder().CreateCall(
                    jit->get_module()->getFunction("create_elements"),
                    create_elements_args);
            inputs_to_next_stage = elements;
            llvm_stage_params.push_back(elements);
            llvm_stage_params.push_back(num_elements_to_create);
        } else { // no output, just forward the input
            inputs_to_next_stage = llvm_stage_params[0];
        }
    } else { // some other stage
        llvm_stage_params = get_all_element_params(jit, pipeline, llvm_element_type);
        std::vector<llvm::Value *> create_elements_args;
        llvm::Value *num_elements_to_create = llvm_stage_params[1];
        create_elements_args.push_back(num_elements_to_create);
        create_elements_args.push_back(as_i1(false));
        llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"),
                                                              create_elements_args);
        inputs_to_next_stage = elements;
        llvm_stage_params.push_back(elements);
        llvm_stage_params.push_back(num_elements_to_create);
    }

    // add the fields that correspond to the first stage (if necessary)
    int field_idx = 0;
    for (size_t i = 0; i < output_fields.size(); i++) {
        llvm_stage_params.push_back(fields[field_idx++]);
    }

    // call the first stage
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
    llvm::Value *num_outputs = jit->get_builder().CreateCall(mfunction_stage->get_llvm_stage(), llvm_stage_params);
    jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());

    if (stage->is_filter() || (stage->is_comparison() && !output_fields.empty())) {
        //     Clear out the space allocated for output Element objects that wasn't used because of the input Element objects that were dropped.
        llvm::Value *realloc_size = codegen_llvm_mul(jit, num_outputs, as_i32(sizeof(Element *)));
        llvm::LoadInst *loaded_arg = codegen_llvm_load(jit, inputs_to_next_stage, 8);
        llvm::Value *reallocated = codegen_mrealloc_and_cast(jit, loaded_arg, realloc_size, loaded_arg->getType()); // shrink size
        codegen_llvm_store(jit, reallocated, inputs_to_next_stage, 8); // overrwrite the current pointer that was there
    }

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
        if (stage->is_comparison()) { // duplicate the input setelements
            llvm_stage_params.push_back(inputs_to_next_stage);
            llvm_stage_params.push_back(num_outputs);
            if (!output_fields.empty()) {
                std::vector<llvm::Value *> create_elements_args;
                llvm::Value *num_elements_to_create = codegen_llvm_mul(jit, llvm_stage_params[1], llvm_stage_params[1]);
                create_elements_args.push_back(num_elements_to_create);
                create_elements_args.push_back(as_i1(false));
                llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"),
                                                                      create_elements_args);
                inputs_to_next_stage = elements;
                llvm_stage_params.push_back(elements);
                llvm_stage_params.push_back(num_elements_to_create);
                for (size_t i = field_idx; i < field_idx + num_stage_output_fields; i++) {
                    llvm_stage_params.push_back(fields[i]);
                }
                field_idx += num_stage_output_fields;
            } else { // no output, just forward the input
                inputs_to_next_stage = llvm_stage_params[0];
            }
        } else if (!stage->is_filter() && !output_fields.empty()) { // filter doesn't create new outputs
            std::vector<llvm::Value *> create_element_args;
            if (!stage->is_segmentation()) {
                create_element_args.push_back(num_outputs);
                create_element_args.push_back(as_i1(false));
            } else {
//                num_outputs = ((SegmentationStage *) stage)->compute_num_segments_old(num_outputs, pipeline);
                llvm::AllocaInst *total_num_segments = codegen_llvm_alloca(jit, llvm_int32, 4);
                codegen_llvm_store(jit, as_i32(0), total_num_segments, 4);
                ForLoop loop(jit);
                loop.init_codegen(pipeline);
                jit->get_builder().CreateBr(loop.get_counter_bb());

                // counter (goes to condition)
                jit->get_builder().SetInsertPoint(loop.get_counter_bb());
                llvm::AllocaInst *output_alloc = codegen_llvm_alloca(jit, num_outputs->getType(), 4);
                codegen_llvm_store(jit, num_outputs, output_alloc, 4);
                loop.codegen_counters(output_alloc);
                jit->get_builder().CreateBr(loop.get_condition_bb());

                // condition (goes to seg counter if still in loop, next otherwise)
                llvm::BasicBlock *count = llvm::BasicBlock::Create(llvm::getGlobalContext(), "seg_count", pipeline);
                llvm::BasicBlock *next = llvm::BasicBlock::Create(llvm::getGlobalContext(), "next", pipeline);
                loop.codegen_condition();
                jit->get_builder().CreateCondBr(loop.get_condition()->get_loop_comparison(),
                                                count, next);
                // seg counter. goes to increment
                jit->get_builder().SetInsertPoint(count);
                std::vector<llvm::Value *> segment_params;
                std::vector<llvm::Value *> gep_params;
                gep_params.push_back(codegen_llvm_load(jit, loop.get_loop_idx(), 4));
                segment_params.push_back(codegen_llvm_load(jit, codegen_llvm_gep(jit, inputs_to_next_stage, gep_params), 4));
                llvm::Value *num_segs = jit->get_builder().CreateCall(((SegmentationStage*)stage)->get_compute_num_segments_mfunc(), segment_params);
                codegen_llvm_store(jit, codegen_llvm_add(jit, codegen_llvm_load(jit, total_num_segments, 4), num_segs), total_num_segments, 4);
                jit->get_builder().CreateBr(loop.get_increment_bb());

                // increment. goes to condition
                loop.codegen_loop_idx_increment();
                jit->get_builder().CreateBr(loop.get_condition_bb());
                jit->get_builder().SetInsertPoint(next);

                num_outputs = codegen_llvm_load(jit, total_num_segments, 4);
                create_element_args.push_back(num_outputs);
                create_element_args.push_back(as_i1(false));
            }
            llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"), create_element_args);
            inputs_to_next_stage = elements;
            llvm_stage_params.push_back(elements);
            llvm_stage_params.push_back(num_outputs);
            for (size_t i = field_idx; i < field_idx + num_stage_output_fields; i++) {
                llvm_stage_params.push_back(fields[i]);
            }
            field_idx += num_stage_output_fields;
        } else if (stage->is_filter()) {
            std::vector<llvm::Value *> create_element_args;
            create_element_args.push_back(num_outputs);
            create_element_args.push_back(as_i1(true));
            llvm::Value *elements = jit->get_builder().CreateCall(jit->get_module()->getFunction("create_elements"), create_element_args);
            inputs_to_next_stage = elements;
            llvm_stage_params.push_back(elements);
            llvm_stage_params.push_back(num_outputs);
        }
        // call the current stage
        num_outputs = jit->get_builder().CreateCall(mfunction_stage->get_llvm_stage(), llvm_stage_params);
        jit->get_builder().CreateCall(jit->get_module()->getFunction("print_sep"), std::vector<llvm::Value *>());
        if (stage->is_filter() || (stage->is_comparison() && !output_fields.empty())) {
            //     Clear out the space allocated for output Element objects that wasn't used because of the input Element objects that were dropped.
            llvm::Value *realloc_size = codegen_llvm_mul(jit, num_outputs, as_i32(sizeof(Element *)));
            llvm::LoadInst *loaded_arg = codegen_llvm_load(jit, inputs_to_next_stage, 8);
            llvm::Value *reallocated = codegen_mrealloc_and_cast(jit, loaded_arg, realloc_size, loaded_arg->getType()); // shrink size
            codegen_llvm_store(jit, reallocated, inputs_to_next_stage, 8); // overrwrite the current pointer that was there
        }
    }

    jit->get_builder().CreateRet(nullptr);
    mfunction_stage->verify_stage();
}
