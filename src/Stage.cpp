//
// Created by Jessica Ray on 1/28/16.
//

#include "./Stage.h"
#include "./Field.h"

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

void Stage::init_stage() {
    MType *set_element_type = create_type<SetElement>();
    MType *set_element_ptr_type = new MPointerType(set_element_type);
    MPointerType *set_element_ptr_ptr_type = new MPointerType(set_element_ptr_type);

    mtypes_to_delete.push_back(set_element_ptr_type);
    mtypes_to_delete.push_back(set_element_ptr_ptr_type);

    // inputs to the user function
    // this would be a single input setelement and output setelement in most cases
    std::vector<MType *> user_function_param_types;
    user_function_param_types.push_back(set_element_ptr_type);
    if (!is_filter()) {
        if (!is_segmentation()) {
            // give the user an array of pointers to hold all of the generated segments
            user_function_param_types.push_back(set_element_ptr_type);
        } else {
            user_function_param_types.push_back(set_element_ptr_ptr_type);
        }
    }

    std::vector<MType *> stage_param_types;
    stage_param_types.push_back(set_element_ptr_ptr_type); // the input SetElement objects
    stage_param_types.push_back(MScalarType::get_int_type()); // the number of input SetElement objects
    stage_param_types.push_back(set_element_ptr_ptr_type); // the output SetElement objects
    stage_param_types.push_back(MScalarType::get_int_type()); // the number of output SetElement objects
    // add the types of the output relation fields
    // we need these fields so that space in them can be preallocated
    for (std::vector<BaseField *>::iterator iter = output_relation_field_types.begin();
         iter != output_relation_field_types.end(); iter++) {
        MType *field_ptr_type = new MPointerType((*iter)->to_mtype());
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

    MFunc *func = new MFunc(user_function_name, stage_name, user_function_return_type, return_type_str_ptr,
                            user_function_param_types, stage_param_types, jit);

    set_function(func);

    loop = new ForLoop(jit, mfunction);
    stage_arg_loader = new StageArgLoaderIB();
    user_function_arg_loader = new UserFunctionArgLoaderIB();
    call = new ExternCallIB();
    preallocator = new FixedPreallocatorIB();
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
        stage_arg_loader->insert(mfunction->get_extern_wrapper());
        stage_arg_loader->codegen(jit);
        llvm::AllocaInst *loop_bound_alloc = stage_arg_loader->get_num_data_structs();

        // Create the object that will load the current inputs for the extern function
        user_function_arg_loader->insert(mfunction->get_extern_wrapper());

        // Create the object that will call the extern function
        call->insert(mfunction->get_extern_wrapper());
        call->set_extern_function(mfunction->get_extern());

        // This block is for cleanup and exiting the stage.
        llvm::BasicBlock *stage_end = llvm::BasicBlock::Create(llvm::getGlobalContext(), "stage_end",
                                                               mfunction->get_extern_wrapper());

        // Create the for loop that will run the stage inputs through the extern function
        loop->init_codegen();
        jit->get_builder().CreateBr(loop->get_counter_bb());
        loop->codegen_counters(loop_bound_alloc);

        // This value is used to keep track of the total number of array elements that will be dumped out of this stage
        // once the loop is completely executed. For example, if the output of the stage is of type FloatElement,
        // then each individual FloatElement has a data array float[]. If M total FloatElements are returned from this
        // stage, then the sum of the lengths of all M float[] arrays is stored in here. This value is needed for preallocation
        // of later stages. It can usually be calculated just from the input values, but in the case of a FilterStage,
        // it has to be computed on the fly since we don't know which FloatElements (or w/e type) will be kept ahead of time.
        // TODO move this to the loop counter thing
        llvm::AllocaInst *output_data_array_size = jit->get_builder().CreateAlloca(llvm::Type::getInt32Ty(llvm::getGlobalContext()), nullptr, "output_size");
        jit->get_builder().CreateStore(CodegenUtils::get_i32(0), output_data_array_size);
        llvm::LoadInst *loop_bound = jit->get_builder().CreateLoad(loop_bound_alloc);

        // preallocate the space
        preallocate();
        jit->get_builder().CreateBr(loop->get_condition_bb());

        // Create the loop index check
        loop->codegen_condition();
        jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(),
                                        user_function_arg_loader->get_basic_block(),
                                        stage_end);

        // Create the loop index increment
        loop->codegen_loop_idx_increment();
        jit->get_builder().CreateBr(loop->get_condition_bb());

        // Process the data through the user function in the loop body
        // Get the current input to the user function
        user_function_arg_loader->set_loop_idx_alloc(get_user_function_arg_loader_idxs());
        if (is_filter()) {
            user_function_arg_loader->set_no_output_param();
        }
        user_function_arg_loader->set_preallocated_output_space(preallocator->get_preallocated_space());
        user_function_arg_loader->set_stage_input_arg_alloc(get_user_function_arg_loader_data());
        user_function_arg_loader->set_output_idx_alloc(loop->get_return_idx());
        if (is_segmentation()) {
            user_function_arg_loader->set_segmentation_stage();
        }
        user_function_arg_loader->codegen(jit);
        jit->get_builder().CreateBr(call->get_basic_block());

        // Call the extern function
        call->set_extern_arg_allocs(user_function_arg_loader->get_user_function_input_allocs());
        call->codegen(jit);
        handle_extern_output(output_data_array_size);
        jit->get_builder().CreateBr(loop->get_increment_bb());

        // Finish up the stage and create the final stage output.
        jit->get_builder().SetInsertPoint(stage_end);
        llvm::AllocaInst *final_stage_output = finish_stage(output_data_array_size, 0);

        // exit
        jit->get_builder().CreateRet(jit->get_builder().CreateLoad(final_stage_output));
        codegen_done = true;
    }
}

std::vector<llvm::AllocaInst *> Stage::get_user_function_arg_loader_idxs() {
    std::vector<llvm::AllocaInst *> idxs;
    idxs.push_back(loop->get_loop_idx());
    return idxs;
}

std::vector<llvm::AllocaInst *> Stage::get_user_function_arg_loader_data() {
    std::vector<llvm::AllocaInst *> data;
    data.push_back(stage_arg_loader->get_data(0)); // input SetElement
    data.push_back(stage_arg_loader->get_data(2)); // output SetElement
    return data;
}



void Stage::handle_extern_output(llvm::AllocaInst *output_data_array_size) {
    loop->codegen_return_idx_increment();
}

llvm::Value *Stage::compute_preallocation_data_array_size(unsigned int fixed_size) {
//    if (is_fixed_size) {
    return jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop->get_loop_bound()),
                                        CodegenUtils::get_i32(fixed_size));
//    } else {
//        return jit->get_builder().CreateLoad(stage_arg_loader->get_data_array_size());
//    }
}

llvm::AllocaInst *Stage::finish_stage(llvm::AllocaInst *output_data_array_size, unsigned int fixed_size) {
    // The final stage output is a struct wrapping the computed BaseElements, the number of output BaseElements,
    // and the total x_dimension of all the arrays in the output BaseElements.
    llvm::Type *llvm_return_type = mfunction->get_extern_wrapper_return_type()->codegen_type();
    llvm::AllocaInst *final_stage_output = jit->get_builder().CreateAlloca(llvm_return_type);
    // RetStruct *rs = (RetStruct*)malloc(sizeof(RetStruct)), RetStruct = { { i64, i64, float* }**, i64, i64 }, so sizeof = 24
    llvm::Value *stage_output_malloc = CodegenUtils::codegen_c_malloc32_and_cast(jit, 24, llvm_return_type);
    jit->get_builder().CreateStore(stage_output_malloc, final_stage_output);
    llvm::LoadInst *final_stage_output_load = jit->get_builder().CreateLoad(final_stage_output);
    llvm::LoadInst *filled_preallocated_space = jit->get_builder().CreateLoad(preallocator->get_preallocated_space());

    // The space we preallocated earlier is now filled with the results of the extern function, so we have to save it in
    // this final_stage_output struct.
    llvm::Value *output_set_elements = CodegenUtils::gep(jit, final_stage_output_load, 0, 0);
    jit->get_builder().CreateStore(jit->get_builder().CreateLoad(stage_arg_loader->get_args_alloc()[2]), output_set_elements); // pass along the output set elements
//    jit->get_builder().CreateStore(filled_preallocated_space, output_set_elements);
//    llvm::Value *data_array_size = CodegenUtils::gep(jit, final_stage_output_load, 0, 1);
    llvm::Value *num_output_setelements = CodegenUtils::gep(jit, final_stage_output_load, 0, 1);

//    if (!is_filter()) {
//        if (is_fixed_size) {
//        llvm::Value *data_array_size_comp = jit->get_builder().CreateMul(jit->get_builder().CreateLoad(compute_num_output_structs()), //jit->get_builder().CreateLoad(loop->get_loop_bound()),
//                                                                         CodegenUtils::get_i32(fixed_size));
//        jit->get_builder().CreateStore(data_array_size_comp, output_data_array_size);
//        } else {
//            llvm::LoadInst *input_data_array_size = jit->get_builder().CreateLoad(stage_arg_loader->get_data_array_size());
//            jit->get_builder().CreateStore(input_data_array_size, output_data_array_size);
//        }
//    }
//    finalize_data_array_size(output_data_array_size);
//    jit->get_builder().CreateStore(jit->get_builder().CreateLoad(output_data_array_size), data_array_size);
    jit->get_builder().CreateStore(jit->get_builder().CreateLoad(loop->get_return_idx()), num_output_setelements); // store the number of output SetElements
    return final_stage_output;
}

llvm::AllocaInst *Stage::compute_num_output_structs() {
    return loop->get_loop_bound();
}

void Stage::preallocate() {
    // The preallocated space for the outputs of this stage.
    // Note: For a FilterStage, since we don't know how many inputs will actually be kept in the output, we
    // preallocate enough space to store all of the inputs.
    preallocator->insert(mfunction->get_extern_wrapper());
    jit->get_builder().CreateBr(preallocator->get_basic_block());
    jit->get_builder().SetInsertPoint(preallocator->get_basic_block());
    for (std::vector<BaseField *>::iterator iter = output_relation_field_types.begin(); // preallocate will show up numerous times now because we have multiple output setelements
         iter != output_relation_field_types.end(); iter++) {
        preallocator->set_base_type(*iter);
        preallocator->set_num_output_structs_alloc(compute_num_output_structs());
        preallocator->set_fixed_size((*iter)->get_fixed_size());
        preallocator->set_data_array_size(compute_preallocation_data_array_size((*iter)->get_fixed_size()));
        preallocator->codegen(jit, true);
    }
}

void Stage::finalize_data_array_size(llvm::AllocaInst *output_data_array_size) { }