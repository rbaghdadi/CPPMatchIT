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
#include "./InstructionBlock.h"
#include "./ForLoop.h"
#include "./Utils.h"
#include "./Field.h"

class Stage {

protected:

    JIT *jit;
    MFunc *mfunction;
    std::vector<BaseField *> input_relation_field_types;
    std::vector<BaseField *> output_relation_field_types;
    MType *user_function_return_type;
    bool codegen_done = false;
    std::string stage_name;
    std::string user_function_name;
    std::vector<MType *> mtypes_to_delete;

    // Building blocks for a given stage
    ForLoop *loop;
    StageArgLoader *stage_arg_loader;
    UserFunctionArgLoader *user_function_arg_loader;
    UserFunctionCall *call;
    Preallocator *preallocator;

public:

    // input_type: this is the type I in the subclasses. It's the user type that is going to be fed into the user function
    // output_type: this is the type O in the subclasses. It's the user type of the data that will be output from this stage
    // (not including the additional counters and such that I manually add on)
    // user_function_return_type: this is the output type of the user function call. It will usually be mvoid_type, but in the case of
    // something like filter, it will be mbool_type

    Stage(JIT *jit, std::string stage_name, std::string user_function_name, Fields *input_relation,
          Fields *output_relation, MType *user_function_return_type) :
            jit(jit), input_relation_field_types(input_relation->get_fields()),
            output_relation_field_types(output_relation->get_fields()),
            user_function_return_type(user_function_return_type), stage_name(stage_name),
            user_function_name(user_function_name) { }

    // no output relation
    Stage(JIT *jit, std::string stage_name, std::string user_function_name, Fields *input_relation,
          MType *user_function_return_type) :
            jit(jit), input_relation_field_types(input_relation->get_fields()),
            output_relation_field_types(std::vector<BaseField *>()),
            user_function_return_type(user_function_return_type), stage_name(stage_name),
            user_function_name(user_function_name) { }


    virtual ~Stage() {
        delete mfunction;
        delete loop;
        delete stage_arg_loader;
        delete user_function_arg_loader;
        delete call;
        delete preallocator;
        for (std::vector<MType *>::iterator iter = mtypes_to_delete.begin(); iter != mtypes_to_delete.end(); iter++) {
            delete (*iter);
        }
    }

    void init_stage();

    std::string get_function_name();

    MFunc *get_mfunction();

    std::vector<BaseField *> get_input_relation_field_types();

    std::vector<BaseField *> get_output_field_types();

    void set_function(MFunc *mfunction);

    /**
     * Create the LLVM function prototypes for the stage wrapper and the user function
     */
    virtual void init_codegen();

    /**
     * Generate the final LLVM code for this stage.
     */
    virtual void codegen();

    /**
     * Stages should override this is if the stage outputs something from the user function that needs
     * to be specially handled (such as the boolean output from the FilterStage).
     */
    virtual void handle_user_function_output();

    virtual bool is_filter();

    virtual bool is_transform();

    virtual bool is_segmentation();

    virtual bool is_comparison();

    /**
     * How to compute the amount of space that we need to preallocate
     */
    virtual llvm::Value *compute_preallocation_data_array_size(unsigned int fixed_size);

    /**
     * Preallocate output space based on the type.
     */
    virtual std::vector<llvm::AllocaInst *> preallocate();

    /**
     * The number of output structs returned across all the user function calls.
     * Will usually just be the loop bound value.
     */
    virtual llvm::AllocaInst *compute_num_output_elements();

    /**
     * Can update the number of data array elements to be stored if necessary
     */
//    virtual void finalize_data_array_size(llvm::AllocaInst *output_data_array_size);

    /**
     * Do the final malloc for the output struct, save the outputs to it, return from the stage
     */
    virtual llvm::AllocaInst *finish_stage(unsigned int fixed_size);

    virtual std::vector<llvm::AllocaInst *> get_user_function_arg_loader_idxs();

    virtual std::vector<llvm::AllocaInst *> get_user_function_arg_loader_data();

    virtual void codegen_main_loop(std::vector<llvm::AllocaInst *> preallocated_space,
                                   llvm::BasicBlock *stage_end);

    /**
     * ComparisonStage and FilterStage only pass along results if the user function returns true. This is called from
     * their overriden versions of handle_user_function_output. Can be used by anything in the future that needs filtering too.
     */
    void filter_user_function_output();

};

#endif //MATCHIT_STAGE_H
