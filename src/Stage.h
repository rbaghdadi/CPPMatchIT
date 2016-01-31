//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_STAGE_H
#define MATCHIT_STAGE_H

#include <string>
#include <tuple>
#include "llvm/IR/Function.h"
#include "./MFunc.h"
#include "./CodegenUtils.h"

class InstructionBlock {
protected:

    llvm::BasicBlock *bb;
    bool codegen_done = false;

public:

    virtual ~InstructionBlock() { }

    llvm::BasicBlock *get_basic_block() {
        return bb;
    }

    void set_function(llvm::Function *func) {
        assert(bb);
        bb->insertInto(func);
    }

    virtual void codegen(JIT *jit, bool no_insert = false) = 0;

};

class LoopCounterBasicBlock : public InstructionBlock {

private:

    // key instructions generated in the block
    llvm::AllocaInst *loop_idx;
    llvm::AllocaInst *loop_bound;
    llvm::AllocaInst *return_idx;
    // values needed for this block
    llvm::Value *max_bound;

public:

    ~LoopCounterBasicBlock() { }

    LoopCounterBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.loop_counters");
    }


    llvm::AllocaInst *get_loop_idx() {
        return loop_idx;
    }

    llvm::AllocaInst *get_loop_bound() {
        return loop_bound;
    }

    llvm::AllocaInst *get_return_idx() {
        return return_idx;
    }

    void set_max_bound(llvm::Value *max_bound) {
        this->max_bound = max_bound;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(max_bound);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
        loop_idx = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_idx(jit).get_result());
        loop_bound = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_idx(jit).get_result());
        return_idx = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_idx(jit).get_result());
        llvm::LoadInst *load = jit->get_builder().CreateLoad(max_bound);
        load->setAlignment(8);
        jit->get_builder().CreateStore(load, loop_bound)->setAlignment(8);
        codegen_done = true;
    }

};

class ReturnStructBasicBlock : public InstructionBlock  {

private:

    // key instructions generated in the block
    llvm::AllocaInst *return_struct;
    // values needed for this block
    MType *stage_return_type;
    MFunc *extern_function;
    llvm::AllocaInst *loop_bound;

public:

    ReturnStructBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "return_struct");
    }

    ~ReturnStructBasicBlock() { }

    llvm::AllocaInst *get_return_struct() {
        return return_struct;
    }

    void set_stage_return_type(MType *stage_return_type) {
        this->stage_return_type = stage_return_type;
    }

    void set_extern_function(MFunc *extern_function) {
        this->extern_function = extern_function;
    }

    void set_loop_bound(llvm::AllocaInst *loop_bound) {
        this->loop_bound = loop_bound;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(stage_return_type && extern_function && loop_bound);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
        return_struct = llvm::cast<llvm::AllocaInst>(CodegenUtils::init_return_data_structure(jit, stage_return_type, *extern_function, loop_bound).get_result());
        codegen_done = true;
    }

};

class ForLoopConditionBasicBlock : public InstructionBlock {

private:

    // key instructions generated in the block
    llvm::Value *comparison;
    // values needed for this block
    llvm::AllocaInst *loop_idx;
    llvm::AllocaInst *loop_bound;

public:

    ForLoopConditionBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.loop_condition");
    }

    ~ForLoopConditionBasicBlock() { }

    llvm::Value *get_loop_comparison() {
        return comparison;
    }

    void set_loop_idx(llvm::AllocaInst *loop_idx) {
        this->loop_idx = loop_idx;
    }

    void set_loop_bound(llvm::AllocaInst *loop_bound) {
        this->loop_bound = loop_bound;
    }

    // this should be the last thing called after all the optimizations and such are performed
    void codegen(JIT *jit, bool no_insert = false) {
        assert(loop_idx && loop_bound);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
        comparison = CodegenUtils::create_loop_idx_condition(jit, loop_idx, loop_bound).get_result();
        codegen_done = true;
    }


};

class ForLoopIncrementBasicBlock : public InstructionBlock {

private:

    // values needed for this block
    llvm::AllocaInst *loop_idx;

public:

    ForLoopIncrementBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.increment");
    }

    ~ForLoopIncrementBasicBlock() { }

    void set_loop_idx(llvm::AllocaInst *loop_idx) {
        this->loop_idx = loop_idx;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(loop_idx);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
        CodegenUtils::increment_idx(jit, loop_idx);
        codegen_done = true;
    }

};

class ForLoopEndBasicBlock : public InstructionBlock {

private:

    // values needed for this block
    llvm::AllocaInst *return_struct;
    llvm::AllocaInst *return_idx;

public:

    ForLoopEndBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.end");
    }

    ~ForLoopEndBasicBlock() { }

    void set_return_struct(llvm::AllocaInst *return_struct) {
        this->return_struct = return_struct;
    }

    void set_return_idx(llvm::AllocaInst *return_idx) {
        this->return_idx = return_idx;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(return_struct && return_idx);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
        CodegenUtils::return_data(jit, return_struct, return_idx);
        codegen_done = true;
    }

};

class ExternArgPrepBasicBlock : public InstructionBlock  {

private:

    // key instructions generated in the block
    std::vector<llvm::Value *> args;
    // values needed for this block
    MFunc *extern_function;

public:

    ExternArgPrepBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "arg.prep");
    }

    ~ExternArgPrepBasicBlock() { }

    std::vector<llvm::Value *> get_args() {
        return args;
    }

    void set_extern_function(MFunc *extern_function) {
        this->extern_function = extern_function;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(extern_function);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
        args = CodegenUtils::init_function_args(jit, *extern_function).get_results();
        codegen_done = true;
    }

};

class ExternCallBasicBlock : public InstructionBlock  {

private:

    // key instructions generated in the block
    llvm::AllocaInst *data_to_return;
    // values needed for this block
    MFunc *extern_function;
    std::vector<llvm::Value *> args;

public:

    ExternCallBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "extern.call");
    }

    ~ExternCallBasicBlock() { }

    llvm::AllocaInst *get_data_to_return() {
        return data_to_return;
    }

    void set_extern_function(MFunc *extern_function) {
        this->extern_function = extern_function;
    }

    void set_extern_args(std::vector<llvm::Value *> args) {
        this->args = args;
    }

    // used for a stage like FilterStage which needs to store the input data rather than the output of the filter function
    void override_data_to_return(llvm::AllocaInst *data_to_return) {
        this->data_to_return = data_to_return;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(extern_function);
        assert(!codegen_done);
        if (!no_insert) {
            jit->get_builder().SetInsertPoint(bb);
        }
        data_to_return = llvm::cast<llvm::AllocaInst>(CodegenUtils::create_extern_call(jit, *extern_function, args).get_result());
        codegen_done = true;
    }

};

class ExternCallStoreBasicBlock : public InstructionBlock  {

private:

    // values needed for this block
    MType *return_type;
    llvm::AllocaInst *return_struct;
    llvm::AllocaInst *return_idx;
    llvm::AllocaInst *data_to_store;

public:

    ExternCallStoreBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store");
    }

    ~ExternCallStoreBasicBlock() { }

    void set_mtype(MType *return_type) {
        this->return_type = return_type;
    }

    void set_return_struct(llvm::AllocaInst *return_struct) {
        this->return_struct = return_struct;
    }

    void set_return_idx(llvm::AllocaInst *return_idx) {
        this->return_idx = return_idx;
    }

    void set_data_to_store(llvm::AllocaInst *extern_call_result) {
        this->data_to_store = extern_call_result;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(return_type);
        assert(return_struct);
        assert(return_idx);
        assert(data_to_store);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
//        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(data_to_store->getType());
//        jit->get_builder().CreateStore(data_to_store, alloc);
        CodegenUtils::store_extern_result(jit, return_type, return_struct, return_idx, data_to_store);
        codegen_done = true;
    }

};

class ExternInitBasicBlock : public InstructionBlock {

private:

    // key instructions generated in the block
    llvm::AllocaInst *element;
    // values needed for this block
    llvm::AllocaInst *data;
    llvm::AllocaInst *loop_idx;

public:

    ExternInitBasicBlock() {
        bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "extern.init");
    }

    ~ExternInitBasicBlock() { }

    llvm::AllocaInst *get_element() {
        return element;
    }

    void set_data(llvm::AllocaInst *data) {
        this->data = data;
    }

    void set_loop_idx(llvm::AllocaInst *loop_idx) {
        this->loop_idx = loop_idx;
    }

    void codegen(JIT *jit, bool no_insert = false) {
        assert(data);
        assert(loop_idx);
        assert(!codegen_done);
        jit->get_builder().SetInsertPoint(bb);
        llvm::LoadInst *loaded_data = jit->get_builder().CreateLoad(data);
        llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
        std::vector<llvm::Value *> index;
        index.push_back(cur_loop_idx);
        llvm::Value *element_gep = jit->get_builder().CreateInBoundsGEP(loaded_data, index);
        llvm::LoadInst *loaded_element = jit->get_builder().CreateLoad(element_gep);
        element = jit->get_builder().CreateAlloca(loaded_element->getType());
        jit->get_builder().CreateStore(loaded_element, element);
        codegen_done = true;
    }

};


// TODO should probably be a struct to be easier to read, work with
// TODO please fix this soon :)
typedef std::tuple<llvm::BasicBlock *, llvm::BasicBlock *, llvm::AllocaInst *, llvm::AllocaInst *, llvm::AllocaInst *, llvm::Value *, llvm::AllocaInst *> generic_codegen_tuple;

class Stage {

protected:

    // these reference the wrapper functions, not the extern function that is what actually does work
    JIT *jit;
    MFunc *mfunction;
    std::string function_name;
    mtype_code_t input_mtype_code;
    mtype_code_t output_mtype_code;

    LoopCounterBasicBlock *loop_counter_basic_block;
    ReturnStructBasicBlock *return_struct_basic_block;
    ForLoopConditionBasicBlock *for_loop_condition_basic_block;
    ForLoopIncrementBasicBlock *for_loop_increment_basic_block;
    ForLoopEndBasicBlock *for_loop_end_basic_block;
    ExternInitBasicBlock *extern_init_basic_block;
    ExternArgPrepBasicBlock *extern_arg_prep_basic_block;
    ExternCallBasicBlock *extern_call_basic_block;
    ExternCallStoreBasicBlock *extern_call_store_basic_block;
    llvm::BasicBlock *dummy_block;

    /**
     * Initialize the return value of extern_func_wrapper that will be filled on each iteration as extern_func is called.
     *
     * @param entry_block The block that is to be run right before the loop is entered.
     * @return Allocated space for the return value of extern_wrapper.
     */
//    llvm::AllocaInst *codegen_init_ret_stack();

    /**
     * Initialize the index for storing the most recently computed return value. Used for block functions like jpg_filter
     * which may remove data, thus returning a different amount of data than was passed in.
     *
     * @param entry_block The block that is to be run right before the loop is entered.
     */
//    llvm::AllocaInst *codegen_init_loop_ret_idx();

    /**
     * Initialize the input data that will be fed into extern_func as arguments
     *
     * @param entry_block The block that is run right before the loop is entered.
     * @return LLVM versions of the args that need to be passed into extern_func.
     */
//    std::vector<llvm::AllocaInst *> codegen_init_args();

    /**
     * Create the section of code that will run after the for loop is done running extern_func.
     * It returns the computed data.
     *
     * @param end_block The block that is to be run after the loop is finished.
     * @param alloc_ret Allocated space for the return value of extern_wrapper.
     * @param alloc_ret_idx Allocated space for the return value index.
     */
//    void codegen_loop_end_block(llvm::BasicBlock *end_block, llvm::AllocaInst *alloc_ret, llvm::AllocaInst *alloc_ret_idx);

    /**
     * Build the body of extern_wrapper. This creates a for loop around extern_func, processing all the input
     * data through it.
     *
     * @param body_block The block that is to be run within the body of the for loop.
     * @param alloc_loop_idx Allocated space for the loop index.
     * @param alloc_ret Allocated space for the return value of extern_wrapper.
     * @param args allocated space for the LLVM versions of the args that need to be passed into extern_func.
     */
//    llvm::Value *codegen_loop_body_block(llvm::AllocaInst *alloc_loop_idx, std::vector<llvm::AllocaInst *> args);

//    llvm::Value *codegen_init_ret_heap();

//    llvm::AllocaInst *codegen_init_loop_max_bound();

    // allocate space for the results with the ret struct
//    llvm::Value *codegen_init_ret_data_heap(llvm::AllocaInst *max_loop_bound);

//    generic_codegen_tuple codegen_init();

public:

    Stage(JIT *jit, mtype_code_t input_type_code, mtype_code_t output_type_code, std::string function_name) :
            jit(jit), function_name(function_name), input_mtype_code(input_type_code), output_mtype_code(output_type_code) {
        loop_counter_basic_block = new LoopCounterBasicBlock();
        return_struct_basic_block = new ReturnStructBasicBlock();
        for_loop_condition_basic_block = new ForLoopConditionBasicBlock();
        for_loop_increment_basic_block = new ForLoopIncrementBasicBlock();
        for_loop_end_basic_block = new ForLoopEndBasicBlock();
        extern_init_basic_block = new ExternInitBasicBlock();
        extern_arg_prep_basic_block = new ExternArgPrepBasicBlock();
        extern_call_basic_block = new ExternCallBasicBlock();
        extern_call_store_basic_block = new ExternCallStoreBasicBlock();
        dummy_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy");
//        jit->get_builder().SetInsertPoint(dummy_block);
//        jit->get_builder().CreateBr(extern_call_store_basic_block->get_basic_block());
    }

    virtual ~Stage() {
        delete loop_counter_basic_block;
        delete return_struct_basic_block;
        delete for_loop_condition_basic_block;
        delete for_loop_increment_basic_block;
        delete for_loop_end_basic_block;
        delete extern_init_basic_block;
        delete extern_arg_prep_basic_block;
        delete extern_call_basic_block;
        delete extern_call_store_basic_block;
    }

    std::string get_function_name();

    MFunc *get_mfunction();

    void set_function(MFunc *mfunction);

    virtual void codegen() =0;

    mtype_code_t get_input_mtype_code() {
        return input_mtype_code;
    }

    mtype_code_t get_output_mtype_code() {
        return output_mtype_code;
    }

    LoopCounterBasicBlock *get_loop_counter_basic_block() {
        return loop_counter_basic_block;
    }

    ReturnStructBasicBlock *get_return_struct_basic_block() {
        return return_struct_basic_block;
    }

    ForLoopConditionBasicBlock *get_for_loop_condition_basic_block() {
        return for_loop_condition_basic_block;
    }

    ForLoopIncrementBasicBlock *get_for_loop_increment_basic_block() {
        return for_loop_increment_basic_block;
    }

    ForLoopEndBasicBlock *get_for_loop_end_basic_block() {
        return for_loop_end_basic_block;
    }

    ExternInitBasicBlock *get_extern_init_basic_block() {
        return extern_init_basic_block;
    }

    ExternArgPrepBasicBlock *get_extern_arg_prep_basic_block() {
        return extern_arg_prep_basic_block;
    }

    ExternCallBasicBlock *get_extern_call_basic_block() {
        return extern_call_basic_block;
    }

    ExternCallStoreBasicBlock *get_extern_call_store_basic_block() {
        return extern_call_store_basic_block;
    }

    llvm::BasicBlock *get_dummy_block() {
        return dummy_block;
    }

    virtual void postprocess(Stage *stage, llvm::BasicBlock *branch_from, llvm::BasicBlock *branch_into) = 0;

};

#endif //MATCHIT_STAGE_H
