//
// Created by Jessica Ray on 1/28/16.
//

#include "./Stage.h"

std::string Stage::get_function_name() {
    return function_name;
}

MFunc *Stage::get_mfunction() {
    return mfunction;
}

void Stage::set_function(MFunc *mfunction) {
    loop = new ForLoop(jit);
    this->mfunction = mfunction;
    loop->set_mfunction(mfunction);
}

void Stage::set_for_loop(ForLoop *loop) {
    this->loop = loop;
}

mtype_code_t Stage::get_input_mtype_code() {
    return input_mtype_code;
}

mtype_code_t Stage::get_output_mtype_code() {
    return output_mtype_code;
}

WrapperOutputStructIB *Stage::get_return_struct() {
    return return_struct;
}

ForLoopEndIB *Stage::get_for_loop_end() {
    return for_loop_end;
}

ExternArgLoaderIB *Stage::get_extern_init() {
    return extern_init;
}

WrapperArgLoaderIB *Stage::get_load_input_args() {
    return load_input_args;
}

ExternCallIB *Stage::get_extern_call() {
    return extern_call;
}

ExternCallStoreIB *Stage::get_extern_call_store() {
    return extern_call_store;
}

llvm::BasicBlock *Stage::branch_to_after_store() {
    return loop->get_for_loop_increment_basic_block()->get_basic_block();
}


void Stage::codegen() {
    if (!codegen_done) {
        // create the extern and extern wrapper prototypes and do any other stage specific initialization
        init_codegen();

        // initialize the function args
        load_input_args->set_mfunction(mfunction);
        load_input_args->codegen(jit, false);
        jit->get_builder().CreateBr(loop->get_loop_counter_basic_block()->get_basic_block());

        // loop components
        loop->set_max_loop_bound(llvm::cast<llvm::AllocaInst>(last(load_input_args->get_args_alloc())));
        loop->set_branch_to_after_counter(return_struct->get_basic_block());
        loop->set_branch_to_true_condition(extern_init->get_basic_block());
        loop->set_branch_to_false_condition(for_loop_end->get_basic_block());
        loop->codegen();

        // allocate space for the return structure
        return_struct->set_mfunction(mfunction);
        return_struct->set_malloc_size_alloc(loop->get_loop_counter_basic_block()->get_malloc_size_alloc());
        if (mfunction->get_associated_block() == "ComparisonStage") {
            jit->get_builder().SetInsertPoint(return_struct->get_basic_block());
            // output size is N^2 where N is the loop bound
            llvm::LoadInst *loop_bound = jit->get_builder().CreateLoad(
                    loop->get_loop_counter_basic_block()->get_max_loop_bound_alloc());
            llvm::Value *mult = jit->get_builder().CreateMul(loop_bound, loop_bound);
            llvm::AllocaInst *mult_alloc = jit->get_builder().CreateAlloca(mult->getType());
            jit->get_builder().CreateStore(mult, mult_alloc)->setAlignment(8);
            return_struct->set_max_loop_bound_alloc(mult_alloc);
        } else {
            return_struct->set_max_loop_bound_alloc(loop->get_loop_counter_basic_block()->get_max_loop_bound_alloc());
        }
        return_struct->codegen(jit, false);
        jit->get_builder().CreateBr(loop->get_for_loop_condition_basic_block()->get_basic_block());

        // codegen the body for the appropriate stage
        stage_specific_codegen(load_input_args->get_args_alloc(), extern_init, extern_call,
                               extern_call_store->get_basic_block(),
                               loop->get_loop_counter_basic_block()->get_loop_idx_alloc());

        // store the result
        extern_call_store->set_mfunction(mfunction);
        extern_call_store->set_data_to_store(extern_call->get_extern_call_result_alloc());
        extern_call_store->set_output_idx_alloc(loop->get_loop_counter_basic_block()->get_output_idx_alloc());
        extern_call_store->set_wrapper_output_struct_alloc(return_struct->get_wrapper_output_struct_alloc());
        extern_call_store->set_malloc_size(loop->get_loop_counter_basic_block()->get_malloc_size_alloc());
        extern_call_store->codegen(jit, false);
        jit->get_builder().CreateBr(branch_to_after_store());

        // return the data
        for_loop_end->set_mfunction(mfunction);
        for_loop_end->set_wrapper_output_struct(return_struct->get_wrapper_output_struct_alloc());
        for_loop_end->set_output_idx_alloc(loop->get_loop_counter_basic_block()->get_output_idx_alloc());
        for_loop_end->codegen(jit, false);

        mfunction->verify_wrapper();
        codegen_done = true;
    }
}