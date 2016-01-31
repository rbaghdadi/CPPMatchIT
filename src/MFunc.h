//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MFUNC_H
#define MATCHIT_MFUNC_H

#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include "llvm/IR/Function.h"
#include "./MType.h"
#include "./JIT.h"

#define BUFFER_SIZE 10

class MFunc {

private:

    /**
     * Name of the user-defined extern function.
     */
    std::string extern_name;

    /**
     * Name of the wrapper function around the user-defined extern function.
     */
    std::string extern_wrapper_name;

    /**
     * The type of block that this function belongs to (i.e. FilterStage, TransformStage, etc)
     */
    std::string associated_block;

    /**
     * The user-defined extern function.
     */
    llvm::Function *extern_func;

    /**
     * The wrapper around extern_func.
     */
    llvm::Function *extern_wrapper;

    /**
     * MatchIT's return type of extern_func.
     */
    MType *extern_ret_type;

    /**
     * MatchIT version of the data element returned by the wrapper (so doesn't include the index returned)
     */
    MType *extern_wrapper_data_ret_type;

    /**
     * MatchIT's argument types of extern_func.
     */
    std::vector<MType *> extern_arg_types;

    /**
     * Our JIT engine
     */
    JIT *jit;

//    /**
//     * Initialize the return value of extern_func_wrapper that will be filled on each iteration as extern_func is called.
//     *
//     * @param entry_block The block that is to be run right before the loop is entered.
//     * @return Allocated space for the return value of extern_wrapper.
//     */
//    llvm::AllocaInst *init_loop_ret(llvm::BasicBlock *entry_block);
//
//    /**
//     * Initialize the index for storing the most recently computed return value. Used for block functions like jpg_filter
//     * which may remove data, thus returning a different amount of data than was passed in.
//     *
//     * @param entry_block The block that is to be run right before the loop is entered.
//     */
//    llvm::AllocaInst *init_loop_ret_idx(llvm::BasicBlock *entry_block);
//
//    /**
//     * Initialize the input data that will be fed into extern_func as arguments
//     *
//     * @param entry_block The block that is run right before the loop is entered.
//     * @return LLVM versions of the args that need to be passed into extern_func.
//     */
//    std::vector<llvm::Value *> init_args(llvm::BasicBlock *entry_block);
//
//    /**
//     * Create the section of code that will run after the for loop is done running extern_func.
//     * It returns the computed data.
//     *
//     * @param end_block The block that is to be run after the loop is finished.
//     * @param alloc Allocated space for the return value of extern_wrapper.
//     */
//    void build_loop_end_block(llvm::BasicBlock *end_block, llvm::AllocaInst *alloc);
//
//    /**
//     * Build the body of extern_wrapper. This creates a for loop around extern_func, processing all the input
//     * data through it.
//     *
//     * @param body_block The block that is to be run within the body of the for loop.
//     * @param alloc_loop_idx Allocated space for the loop index.
//     * @param alloc_ret Allocated space for the return value of extern_wrapper.
//     * @param args LLVM versions of the args that need to be passed into extern_func.
//     */
//    void build_loop_body_block(llvm::BasicBlock *body_block, llvm::AllocaInst *alloc_loop_idx,
//                               llvm::AllocaInst *alloc_ret, std::vector<llvm::Value *> *args);

public:

    MFunc(std::string name, std::string associated_block, MType *ret_type, std::vector<MType *> arg_types, JIT *jit) :
            extern_name(name), extern_wrapper_name("__execute_" + name), associated_block(associated_block), extern_ret_type(ret_type), extern_arg_types(arg_types), jit(jit) {
//        build_extern_proto();
//        build_extern_wrapper_proto();
//        build_extern_wrapper_body();
//        verify_wrapper();
    }

    ~MFunc() {}

    /**
    * Build the LLVM prototype for extern_func.
    */
    void codegen_extern_proto();

    /**
     * Build the LLVM prototype for extern_wrapper.
     */
    void codegen_extern_wrapper_proto();

    /**
     * Build the for loop within extern_wrapper that will go through the input data and run it through extern_func.
     */
//    void build_extern_wrapper_body();

    /**
     * Check that extern_wrapper is "ok" in LLVM's sense.
     */
    void verify_wrapper();

//    void codegen_full() {
//        codegen_extern_proto();
//        codegen_extern_wrapper_proto();
//        build_extern_wrapper_body();
//        verify_wrapper();
//    }

    void dump();

    const std::string &get_extern_name() const;

    const std::string &get_extern_wrapper_name() const;

    llvm::Function *get_extern() const;

    llvm::Function *get_extern_wrapper() const;

    std::vector<MType *> get_arg_types() const;

    MType *get_ret_type() const;

    MType *get_extern_wrapper_data_ret_type() const;

    const std::string get_associated_block() const {
        return associated_block;
    }

};

#endif //MATCHIT_MFUNC_H
