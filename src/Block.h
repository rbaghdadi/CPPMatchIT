//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_BLOCK_H
#define MATCHIT_BLOCK_H

#include <string>
#include <tuple>
#include "llvm/IR/Function.h"
#include "./MFunc.h"

// TODO should probably be a struct to be easier to read, work with
// TODO please fix this soon :)
typedef std::tuple<llvm::BasicBlock *, llvm::BasicBlock *, llvm::AllocaInst *, llvm::AllocaInst *, llvm::AllocaInst *, llvm::Value *, llvm::AllocaInst *> generic_codegen_tuple;

class Block {

protected:

    // these reference the wrapper functions, not the extern function that is what actually does work
    JIT *jit;
    MFunc *mfunction;
    std::string function_name;
    mtype_code_t input_type_code;
    mtype_code_t output_type_code;

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

    Block(JIT *jit, mtype_code_t input_type_code, mtype_code_t output_type_code, std::string function_name) :
            jit(jit), input_type_code(input_type_code), output_type_code(output_type_code), function_name(function_name) { }

    ~Block() {}

    std::string get_function_name();

    MFunc *get_mfunction();

    void set_function(MFunc *mfunction);

    virtual void codegen() =0;

};


#endif //MATCHIT_BLOCK_H
