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

class Stage {

protected:

    JIT *jit;
    MFunc *mfunction;
    MType *stage_input_type;
    MType *stage_return_type;
    bool codegen_done = false;
    bool is_fixed_size;
    unsigned int fixed_size;
    std::string function_name;
    std::string stage_name;
    MType *extern_return_type;
    std::vector<MType *> mtypes_to_delete;

    // Building blocks for a given stage
    ForLoop *loop;
    StageArgLoaderIB *stage_arg_loader;
    ExternArgLoaderIB *extern_arg_loader;
    ExternCallIB *call;
    PreallocatorIB *preallocator;

public:

    // input_type: this is the type I in the subclasses. It's the user type that is going to be fed into the extern function
    // output_type: this is the type O in the subclasses. It's the user type of the data that will be output from this stage
    // (not including the additional counters and such that I manually add on)
    // extern_return_type: this is the output type of the extern call. It will usually be mvoid_type, but in the case of
    // something like filter, it will be mbool_type
    Stage(JIT *jit, std::string stage_name, std::string function_name, MType *input_type, MType *output_type,
          MType *extern_return_type, bool is_fixed_size = false, unsigned int fixed_size = 0) :
            jit(jit), stage_input_type(input_type), stage_return_type(output_type), is_fixed_size(is_fixed_size),
            fixed_size(fixed_size), function_name(function_name), stage_name(stage_name), extern_return_type(extern_return_type) {
    }

    virtual ~Stage() {
        delete mfunction;
        delete loop;
        delete stage_arg_loader;
        delete extern_arg_loader;
        delete call;
        delete preallocator;
        for (std::vector<MType *>::iterator iter = mtypes_to_delete.begin(); iter != mtypes_to_delete.end(); iter++) {
            delete (*iter);
        }
    }

    void init_stage();

    std::string get_function_name();

    MFunc *get_mfunction();

    void set_function(MFunc *mfunction);

    /**
     * Create the LLVM function prototypes for the stage wrapper and the extern function
     */
    virtual void init_codegen();

    /**
     * Generate the final LLVM code for this stage.
     */
    virtual void codegen();

    /**
     * Stages should override this is if the stage outputs something from the extern function that needs
     * to be specially handled (such as the boolean output from the FilterStage).
     */
    virtual void handle_extern_output(llvm::AllocaInst *output_data_array_size);

    virtual bool is_filter();

    virtual bool is_transform();

    virtual bool is_segmentation();

    virtual bool is_comparison();

    /**
     * How to compute the amount of space that we need to preallocate
     */
    virtual llvm::Value *compute_preallocation_data_array_size();

    /**
     * Preallocate output space based on the type.
     */
    virtual void preallocate();

    /**
     * The number of output structs returned across all the extern calls.
     * Will usually just be the loop bound value.
     */
    virtual llvm::AllocaInst *compute_num_output_structs();

    /**
     * Can update the number of data array elements to be stored if necessary
     */
    virtual void finalize_data_array_size(llvm::AllocaInst *output_data_array_size);

    /**
     * Do the final malloc for the output struct, save the outputs to it, return from the stage
     */
    virtual llvm::AllocaInst *finish_stage(llvm::AllocaInst *output_data_array_size);

    JIT *get_jit();

    virtual std::vector<llvm::AllocaInst *> get_extern_arg_loader_idxs();

    virtual std::vector<llvm::AllocaInst *> get_extern_arg_loader_data();

};

#endif //MATCHIT_STAGE_H
