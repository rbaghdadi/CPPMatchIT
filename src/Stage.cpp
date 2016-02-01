//
// Created by Jessica Ray on 1/28/16.
//

#include "Stage.h"

std::string Stage::get_function_name() {
    return function_name;
}

MFunc *Stage::get_mfunction() {
    return mfunction;
}

void Stage::set_function(MFunc *mfunction) {
    this->mfunction = mfunction;
}

mtype_code_t Stage::get_input_mtype_code() {
    return input_mtype_code;
}

mtype_code_t Stage::get_output_mtype_code() {
    return output_mtype_code;
}

LoopCounterBasicBlock *Stage::get_loop_counter_basic_block() {
    return loop_counter_basic_block;
}

ReturnStructBasicBlock *Stage::get_return_struct_basic_block() {
    return return_struct_basic_block;
}

ForLoopConditionBasicBlock *Stage::get_for_loop_condition_basic_block() {
    return for_loop_condition_basic_block;
}

ForLoopIncrementBasicBlock *Stage::get_for_loop_increment_basic_block() {
    return for_loop_increment_basic_block;
}

ForLoopEndBasicBlock *Stage::get_for_loop_end_basic_block() {
    return for_loop_end_basic_block;
}

ExternInitBasicBlock *Stage::get_extern_init_basic_block() {
    return extern_init_basic_block;
}

ExternArgPrepBasicBlock *Stage::get_extern_arg_prep_basic_block() {
    return extern_arg_prep_basic_block;
}

ExternCallBasicBlock *Stage::get_extern_call_basic_block() {
    return extern_call_basic_block;
}

ExternCallStoreBasicBlock *Stage::get_extern_call_store_basic_block() {
    return extern_call_store_basic_block;
}
