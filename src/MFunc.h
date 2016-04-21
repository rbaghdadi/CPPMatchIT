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
     * Name of the user function.
     */
    std::string user_function_name;

    /**
     * Name of the stage function that wraps the user function.
     */
    std::string stage_name;

    /**
     * The user function.
     */
    llvm::Function *user_function;

    /**
     * The function that wraps a user function.
     */
    llvm::Function *stage_function;

    /**
     * Return type of the user function.
     */
    MType *user_function_return_type;

    /**
     * Return type of the individual elements returned by our stage.
     * Currently, it does not include the int that gets added on to count
     * the number of elements in the output array.
     * The stage itself returns a pointer to this type because we return an array of these.
     */
    MType *stage_return_type;

    /**
     * Types of the parameters to the user function.
     */
    std::vector<MType *> user_function_param_types;

    /**
     * Types of the parameters to the user function.
     */
    std::vector<MType *> stage_param_types;

    /**
     * Our JIT engine
     */
    JIT *jit;

public:

    MFunc(std::string name, std::string associated_block, MType *user_function_return_type, MType *stage_return_type,
          std::vector<MType *> user_function_param_types, std::vector<MType *> stage_param_types, JIT *jit) :
            user_function_name(name), stage_name("__execute_" + name),
            user_function_return_type(user_function_return_type), stage_return_type(stage_return_type),
            user_function_param_types(user_function_param_types), stage_param_types(stage_param_types), jit(jit) { }

    ~MFunc() {}

    /**
    * Build the LLVM prototype for user function.
    */
    void codegen_user_function_proto();

    /**
     * Build the LLVM prototype for stage.
     */
    void codegen_stage_proto();

    /**
     * Check that stage is "ok" in LLVM's sense.
     */
    void verify_stage();

    /**
     * Print out components of this MFunc
     */
    void dump();

    /**
     * Get the name of the user function.
     */
    const std::string &get_user_function_name() const;

    /**
     * Get the name of the stage.
     */
    const std::string &get_stage_name() const;

    /**
     * Get the llvm function for the user function.
     */
    llvm::Function *get_llvm_user_function() const;

    /**
     * Get the llvm function for the stage.
     */
    llvm::Function *get_llvm_stage() const;

    /**
     * Get the parameter types to the user function.
     */
    std::vector<MType *> get_user_function_param_types() const;

    /**
     * Get the parameter types to the stage.
     */
    std::vector<MType *> get_stage_param_types() const;

    /**
     * Get the return type of the user function.
     */
    MType *get_user_function_return_type() const;

    /**
     * Get the return type of the stage.
     */
    MType *get_stage_return_type() const;

};

#endif //MATCHIT_MFUNC_H
