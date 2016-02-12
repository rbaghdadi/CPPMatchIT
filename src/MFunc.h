//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MFUNC_H
#define MATCHIT_MFUNC_H

#include <vector>
#include <string>
#include "llvm/IR/Function.h"
#include "./MType.h"
#include "./JIT.h"

class MFunc {

private:

    /**
     * Name of the extern function.
     */
    std::string extern_name;

    /**
     * Name of the wrapper function around the extern function.
     */
    std::string extern_wrapper_name;

    /**
     * The type of block that this function belongs to (i.e. FilterStage, TransformStage, etc)
     */
    std::string associated_block;

    /**
     * The extern function.
     */
    llvm::Function *extern_function;

    /**
     * The wrapper around extern_func.
     */
    llvm::Function *extern_wrapper_function;

    /**
     * Return type of the extern function.
     */
    MType *extern_return_type;

    /**
     * Return type of the individual elements returned by our wrapper function.
     * Currently, it does not include the int that gets added on to count
     * the number of elements in the output array.
     * The stage itself returns a pointer to this type because we return an array of these.
     */
    MType *extern_wrapper_return_type;

    /**
     * Types of the parameters to the extern function.
     */
    std::vector<MType *> extern_param_types;

    /**
     * Types of the parameters to the extern function.
     */
    std::vector<MType *> extern_wrapper_param_types;

    /**
     * Our JIT engine
     */
    JIT *jit;

public:

    MFunc(std::string name, std::string associated_block, MType *extern_return_type, MType *extern_wrapper_return_type,
          std::vector<MType *> extern_param_types, std::vector<MType *> extern_wrapper_param_types, JIT *jit) :
            extern_name(name), extern_wrapper_name("__execute_" + name), associated_block(associated_block),
            extern_return_type(extern_return_type), extern_wrapper_return_type(extern_wrapper_return_type),
            extern_param_types(extern_param_types), extern_wrapper_param_types(extern_wrapper_param_types), jit(jit) { }

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
     * Check that extern_wrapper is "ok" in LLVM's sense.
     */
    void verify_wrapper();

    /**
     * Print out components of this MFunc
     */
    void dump();

    /**
     * Get the name of the extern function.
     */
    const std::string &get_extern_name() const;

    /**
     * Get the name of the wrapper around the extern function.
     */
    const std::string &get_extern_wrapper_name() const;

    /**
     * Get the llvm function for the extern function.
     */
    llvm::Function *get_extern() const;

    /**
     * Get the llvm function for the wrapper around the extern function.
     */
    llvm::Function *get_extern_wrapper() const;

    /**
     * Get the parameter types to the extern function.
     */
    std::vector<MType *> get_extern_param_types() const;

    /**
     * Get the parameter types to the extern function.
     */
    std::vector<MType *> get_extern_wrapper_param_types() const;


    /**
     * Get the return type of the extern function.
     */
    MType *get_extern_return_type() const;

    /**
     * Get the return type of the wrapper around the extern function.
     */
    MType *get_extern_wrapper_return_type() const;

    /**
     * Get the block associated with this MFunc.
     */
    const std::string get_associated_block() const;

};

#endif //MATCHIT_MFUNC_H
