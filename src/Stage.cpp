//
// Created by Jessica Ray on 1/28/16.
//

#include "./Stage.h"

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

void Stage::init_codegen() {
    mfunction->codegen_extern_proto();
    mfunction->codegen_extern_wrapper_proto();
}

void Stage::codegen() {
    if (!codegen_done) {
        // create the prototypes
        init_codegen();
        jit->dump();

        // Load the inputs to the stage
        stage_arg_loader->insert(mfunction->get_extern_wrapper());
        stage_arg_loader->codegen(jit);
        llvm::AllocaInst *loop_bound_alloc = stage_arg_loader->get_num_data_structs();

        // Create the object that will load the current inputs for the extern function
        extern_arg_loader->insert(mfunction->get_extern_wrapper());

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
        llvm::AllocaInst *output_data_array_size = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
        jit->get_builder().CreateStore(CodegenUtils::get_i64(0), output_data_array_size);
        llvm::LoadInst *loop_bound = jit->get_builder().CreateLoad(loop_bound_alloc);

        // preallocate the space
        preallocate();
        jit->get_builder().CreateBr(loop->get_condition_bb());

        // Create the loop index check
        loop->codegen_condition();
        jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(),
                                        extern_arg_loader->get_basic_block(),
                                        stage_end);

        // Create the loop index increment
        loop->codegen_loop_idx_increment();
        jit->get_builder().CreateBr(loop->get_condition_bb());

        // Process the data through the extern function in the loop body
        // Get the current input to the extern function
        extern_arg_loader->set_loop_idx_alloc(loop->get_loop_idx());
        extern_arg_loader->set_preallocated_output_space(preallocator->get_preallocated_space());
        extern_arg_loader->set_stage_input_arg_alloc(stage_arg_loader->get_data());
        extern_arg_loader->codegen(jit);
        jit->get_builder().CreateBr(call->get_basic_block());

        // Call the extern function
        call->set_extern_arg_allocs(extern_arg_loader->get_extern_input_arg_alloc());
        call->codegen(jit);
        handle_extern_output(output_data_array_size);
        jit->get_builder().CreateBr(loop->get_increment_bb());

        // Finish up the stage and create the final stage output.
        jit->get_builder().SetInsertPoint(stage_end);
        llvm::AllocaInst *final_stage_output = finish_stage(output_data_array_size);

        // exit
        jit->get_builder().CreateRet(jit->get_builder().CreateLoad(final_stage_output));
        codegen_done = true;
    }
}

void Stage::handle_extern_output(llvm::AllocaInst *output_data_array_size) {
    loop->codegen_return_idx_increment();
}

llvm::Value *Stage::compute_data_array_size() {
    if (is_fixed_size) {
        return jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop->get_loop_bound()),
                                                                         CodegenUtils::get_i64(fixed_size));
    } else {
        return jit->get_builder().CreateLoad(stage_arg_loader->get_data_array_size());
    }
}

llvm::AllocaInst *Stage::finish_stage(llvm::AllocaInst *output_data_array_size) {
    // The final stage output is a struct wrapping the computed BaseElements, the number of output BaseElements,
    // and the total length of all the arrays in the output BaseElements.
    llvm::Type *llvm_return_type = mfunction->get_extern_wrapper_return_type()->codegen_type();
    llvm::AllocaInst *final_stage_output = jit->get_builder().CreateAlloca(llvm_return_type);
    // RetStruct *rs = (RetStruct*)malloc(sizeof(RetStruct)), RetStruct = { { i64, i64, float* }**, i64, i64 }, so sizeof = 24
    llvm::Value *stage_output_malloc = CodegenUtils::codegen_c_malloc64_and_cast(jit, 24, llvm_return_type);
    jit->get_builder().CreateStore(stage_output_malloc, final_stage_output);
    llvm::LoadInst *final_stage_output_load = jit->get_builder().CreateLoad(final_stage_output);
    llvm::LoadInst *filled_preallocated_space = jit->get_builder().CreateLoad(preallocator->get_preallocated_space());

    // The space we preallocated earlier is now filled with the results of the extern function, so we have to save it in
    // this final_stage_output struct.
    llvm::Value *data_field = CodegenUtils::gep(jit, final_stage_output_load, 0, 0);
    llvm::Value *data_array_size = CodegenUtils::gep(jit, final_stage_output_load, 0, 1);
    llvm::Value *num_output_base_elements = CodegenUtils::gep(jit, final_stage_output_load, 0, 2);
    jit->get_builder().CreateStore(filled_preallocated_space, data_field);
    if (!is_filter()) {
        if (is_fixed_size) {
            llvm::Value *data_array_size_comp = jit->get_builder().CreateMul(loop->get_loop_bound(),
                                                                             CodegenUtils::get_i64(fixed_size));
            jit->get_builder().CreateStore(data_array_size_comp, output_data_array_size);
        } else {
            llvm::LoadInst *input_data_array_size = jit->get_builder().CreateLoad(stage_arg_loader->get_data_array_size());
            jit->get_builder().CreateStore(input_data_array_size, output_data_array_size);
        }
    }
    jit->get_builder().CreateStore(jit->get_builder().CreateLoad(output_data_array_size), data_array_size);
    jit->get_builder().CreateStore(jit->get_builder().CreateLoad(loop->get_return_idx()), num_output_base_elements);
    return final_stage_output;
}

void Stage::preallocate() {
    // The preallocated space for the outputs of this stage.
    // Note: For a FilterStage, since we don't know how many inputs will actually be kept in the output, we
    // preallocate enough space to store all of the inputs.
    preallocator->insert(mfunction->get_extern_wrapper());
    jit->get_builder().CreateBr(preallocator->get_basic_block());
    jit->get_builder().SetInsertPoint(preallocator->get_basic_block());
    preallocator->set_base_type(stage_return_type);
    preallocator->set_loop_bound_alloc(loop->get_loop_bound());
    if (is_fixed_size) {
        preallocator->set_fixed_size(fixed_size);
    } else { // matched
        // In the matched case, the output_data_array_size is the same as the value that was fed into the stage
        preallocator->set_input_data(stage_arg_loader->get_data());
        preallocator->set_preallocate_outer_only(is_filter());
    }
    preallocator->set_data_array_size(compute_data_array_size());
    preallocator->codegen(jit, true);
}