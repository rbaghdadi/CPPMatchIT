//
// Created by Jessica Ray on 1/28/16.
//

#include "./Stage.h"
#include "./Field.h"

using namespace Codegen;

MFunc *Stage::get_mfunction() {
    return mfunction;
}

void Stage::set_function(MFunc *mfunction) {
    this->mfunction = mfunction;
}

bool Stage::is_filter() {
    return false;
}

bool Stage::is_transform() {
    return false;
}

bool Stage::is_segmentation() {
    return false;
}

bool Stage::is_comparison() {
    return false;
}

std::vector<BaseField *> Stage::get_input_relation_field_types() {
    return input_relation_field_types;
}

std::vector<BaseField *> Stage::get_output_field_types() {
    return output_relation_field_types;
}

void Stage::init_stage() {
    MType *set_element_type = create_type<Element>();
    MType *set_element_ptr_type = new MPointerType(set_element_type);
    MPointerType *set_element_ptr_ptr_type = new MPointerType(set_element_ptr_type);

    mtypes_to_delete.push_back(set_element_ptr_type);
    mtypes_to_delete.push_back(set_element_ptr_ptr_type);

    // inputs to the user function
    // this would be a single input setelement and output setelement in most cases
    std::vector<MType *> user_function_param_types;
    user_function_param_types.push_back(set_element_ptr_type);
    if (is_comparison()) {
        user_function_param_types.push_back(set_element_ptr_type);
        if (!output_relation_field_types.empty()) { // there is an output field, create an output setelement
            user_function_param_types.push_back(set_element_ptr_type);
        }
    } else if (!is_filter()) {
        if (!is_segmentation()) {
            user_function_param_types.push_back(set_element_ptr_type);
        } else {
            // give the user an array of pointers to hold all of the generated segments
            user_function_param_types.push_back(set_element_ptr_ptr_type);
        }
    }

    // inputs to the stage
    std::vector<MType *> stage_param_types;
    stage_param_types.push_back(set_element_ptr_ptr_type); // the input Element objects
    stage_param_types.push_back(MScalarType::get_int_type()); // the number of input Element objects'
    if (is_comparison()) {
        // 2 sets of inputs
        stage_param_types.push_back(set_element_ptr_ptr_type);
        stage_param_types.push_back(MScalarType::get_int_type());
        // possibly some outputs
        if (!output_relation_field_types.empty()) { // there is an output field
            stage_param_types.push_back(set_element_ptr_ptr_type); // the output Element objects
            stage_param_types.push_back(MScalarType::get_int_type()); // the number of output Element objects
        }
    } else { //if (!is_filter()) {
        stage_param_types.push_back(set_element_ptr_ptr_type); // the output Element objects
        stage_param_types.push_back(MScalarType::get_int_type()); // the number of output Element objects
    }
    // add the types of the output relation fields
    // we need these fields so that space in them can be preallocated
    // What kind of preallocation does a filter need? All of the fields are already preallocated, we just need a new list of them
    // I think its just a matter of preallocating a smaller amount of space (enough to just hold a bunch of pointers)
    for (std::vector<BaseField *>::iterator iter = output_relation_field_types.begin();
         iter != output_relation_field_types.end(); iter++) {
        MType *field_ptr_type = new MPointerType(create_type<BaseField>());
        mtypes_to_delete.push_back(field_ptr_type);
        stage_param_types.push_back(field_ptr_type);
    }

    // return type of the stage
    std::vector<MType *> stage_return_type;
    // the output SetElements
    stage_return_type.push_back(set_element_ptr_ptr_type);
    // the number of output SetElements
    stage_return_type.push_back(MScalarType::get_int_type());
    // wrap return type into a struct since we can only have one return object
    MStructType *return_type_struct = new MStructType(mtype_struct, stage_return_type);
    MPointerType *return_type_str_ptr = new MPointerType(return_type_struct);
    mtypes_to_delete.push_back(return_type_struct);
    mtypes_to_delete.push_back(return_type_str_ptr);

    MFunc *func = new MFunc(user_function_name, stage_name, user_function_return_type, MScalarType::get_int_type(),/*return_type_str_ptr,*/
                            user_function_param_types, stage_param_types, jit);

    set_function(func);

    loop = new ForLoop(jit, mfunction);
    stage_arg_loader = new StageArgLoader();
    user_function_arg_loader = new UserFunctionArgLoader();
    call = new UserFunctionCall();
    preallocator = new Preallocator();
}

void Stage::init_codegen() {
    mfunction->codegen_user_function_proto();
    mfunction->codegen_stage_proto();
}

void Stage::codegen() {
    if (!codegen_done) {
        // create the prototypes
        init_codegen();

        // Load the inputs to the stage
        stage_arg_loader->insert(mfunction->get_llvm_stage());
        stage_arg_loader->codegen(jit);
        llvm::AllocaInst *loop_bound_alloc = stage_arg_loader->get_num_input_elements();

        // Create the object that will load the current inputs for the extern function
        user_function_arg_loader->insert(mfunction->get_llvm_stage());

        // Create the object that will call the extern function
        call->insert(mfunction->get_llvm_stage());
        call->set_extern_function(mfunction->get_extern());

        // This block is for cleanup and exiting the stage.
        llvm::BasicBlock *stage_end = llvm::BasicBlock::Create(llvm::getGlobalContext(), "stage_end",
                                                               mfunction->get_llvm_stage());

        // Create the for loop that will run the stage inputs through the extern function
        loop->init_codegen();
        jit->get_builder().CreateBr(loop->get_counter_bb());
        loop->codegen_counters(loop_bound_alloc);

        // preallocate space for all the output fields in the relation OR something else if a Stage overrwrites preallocate()
        std::vector<llvm::AllocaInst *> preallocated_space = preallocate();
        // TODO this is super dangerous!
        if (!is_filter()) {
            std::vector<llvm::Value *> field_data_idxs;
            field_data_idxs.push_back(as_i32(0));
            field_data_idxs.push_back(as_i32(BaseField::get_data_idx())); // the last member of BaseField is the data array
            // store the preallocated space in the field data arrays
            for (int i = 0; i < preallocated_space.size(); i++) {
                // the fields are the last stage args, so pull out those and overwrite their data and size fields
                llvm::AllocaInst *field_alloca = stage_arg_loader->get_args_alloc()[
                        stage_arg_loader->get_args_alloc().size() + i - preallocated_space.size()];
                llvm::LoadInst *dest = codegen_llvm_load(jit, field_alloca, 8);
                llvm::AllocaInst *this_prealloc = preallocated_space[i]; // has preallocated data from malloc
                llvm::LoadInst *src = codegen_llvm_load(jit, this_prealloc, 8);
                // within the field, the last member type is the data array
                llvm::Value *gep = codegen_llvm_gep(jit, dest, field_data_idxs);
                codegen_llvm_store(jit, src, gep, 8);
            }
        }

        // Create the main loop
        codegen_main_loop(preallocated_space, stage_end);

        // Finish up the stage and create the final stage output.
        jit->get_builder().SetInsertPoint(stage_end);
        llvm::AllocaInst *final_stage_output = finish_stage(0);

        // exit
        if (final_stage_output) {
            jit->get_builder().CreateRet(codegen_llvm_load(jit, final_stage_output, 8));
        } else {
            jit->get_builder().CreateRet(nullptr);
        }
        codegen_done = true;
    }
}

// components for processing data through the user function
void Stage::codegen_main_loop(std::vector<llvm::AllocaInst *> preallocated_space,
                                           llvm::BasicBlock *stage_end) {
    jit->get_builder().CreateBr(loop->get_condition_bb());
    loop->codegen_condition();
    jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(),
                                    user_function_arg_loader->get_basic_block(), stage_end);
    // Create the loop index increment
    loop->codegen_loop_idx_increment();
    jit->get_builder().CreateBr(loop->get_condition_bb());

    // Process the data through the user function in the loop body
    // Get the current input to the user function
    // TODO hacky
    std::vector<llvm::AllocaInst *> loop_idxs;
    loop_idxs.push_back(get_user_function_arg_loader_idxs()[0]);
    loop_idxs.push_back(get_user_function_arg_loader_idxs()[0]); // same idx for output as the input
    user_function_arg_loader->set_loop_idx_alloc(loop_idxs);
    if (is_filter()) {
        user_function_arg_loader->set_no_output_param();
    }
    user_function_arg_loader->set_stage_input_arg_alloc(get_user_function_arg_loader_data());
    user_function_arg_loader->set_output_idx_alloc(loop->get_return_idx());
    if (is_segmentation()) {
        user_function_arg_loader->set_segmentation_stage();
    } else if (is_filter()) {
        user_function_arg_loader->set_filter_stage();
    }
    user_function_arg_loader->codegen(jit);
    jit->get_builder().CreateBr(call->get_basic_block());

    // Call the extern function
    call->set_extern_arg_allocs(user_function_arg_loader->get_user_function_input_allocs());
    call->codegen(jit);
    handle_extern_output(preallocated_space);
    jit->get_builder().CreateBr(loop->get_increment_bb());
}

// TODO get rid of this and just inline it
std::vector<llvm::AllocaInst *> Stage::get_user_function_arg_loader_idxs() {
    std::vector<llvm::AllocaInst *> idxs;
    idxs.push_back(loop->get_loop_idx());
    return idxs;
}

std::vector<llvm::AllocaInst *> Stage::get_user_function_arg_loader_data() {
    std::vector<llvm::AllocaInst *> data;
    data.push_back(stage_arg_loader->get_data(0)); // input Element
    // TODO hacky, see comment on the next line
    data.push_back(stage_arg_loader->get_data(2)); // output Element or someother input in the case of comparison
    if (is_comparison() && !output_relation_field_types.empty()) { // check if this is a comparison with an output
        data.push_back(stage_arg_loader->get_data(4));
    }
    return data;
}

// stages that have an extern output (like filter) can do their own thing
void Stage::handle_extern_output(std::vector<llvm::AllocaInst *> preallocated_space) {
    loop->codegen_return_idx_increment(nullptr);
}

llvm::Value *Stage::compute_preallocation_data_array_size(unsigned int fixed_size) {
    return codegen_llvm_mul(jit, codegen_llvm_load(jit, loop->get_loop_bound(), 4), as_i32(fixed_size));
}

llvm::AllocaInst *Stage::finish_stage(unsigned int fixed_size) {
    return loop->get_return_idx();
}

llvm::AllocaInst *Stage::compute_num_output_elements() {
    return loop->get_loop_bound();
}

std::vector<llvm::AllocaInst *> Stage::preallocate() {
    // The preallocated space for the outputs of this stage.
    // Note: For FilterStage/SegmentationStage, since we don't know how many inputs will actually be kept in the output, just
    // preallocate enough space to store all of the inputs.
    std::vector<llvm::AllocaInst *> preallocated_space;
    preallocator->insert(mfunction->get_llvm_stage());
    jit->get_builder().CreateBr(preallocator->get_basic_block());
    jit->get_builder().SetInsertPoint(preallocator->get_basic_block());
    // fields in the output setelements
    // these fields map to the params of the stage (the fields are the last params)
    // preallocate space in each of the individual output fields
    for (std::vector<BaseField *>::iterator iter = output_relation_field_types.begin(); // preallocate will show up numerous times now because we have multiple output fields
         iter != output_relation_field_types.end(); iter++) {
        preallocator->set_base_field(*iter);
        preallocator->set_num_elements_to_alloc(compute_num_output_elements());
        preallocator->codegen(jit, true);
        preallocated_space.push_back(preallocator->get_preallocated_space());
    }
    return preallocated_space;
}