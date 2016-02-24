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
            fixed_size(fixed_size) {

        MPointerType *input_type_ptr = new MPointerType(input_type);
        MPointerType *output_type_ptr = new MPointerType(output_type);
        MPointerType *input_type_ptr_ptr = new MPointerType(input_type_ptr);
        MPointerType *output_type_ptr_ptr = new MPointerType(output_type_ptr);

        mtypes_to_delete.push_back(input_type_ptr);
        mtypes_to_delete.push_back(output_type_ptr);
        mtypes_to_delete.push_back(input_type_ptr_ptr);
        mtypes_to_delete.push_back(output_type_ptr_ptr);

        // inputs to the extern function
        std::vector<MType *> extern_param_types;
        extern_param_types.push_back(input_type_ptr);
        if (!is_filter()) {
            if (!is_segmentation()) {
                // give the user an array of pointers to hold all of the generated segments
                extern_param_types.push_back(output_type_ptr);
            } else {
                extern_param_types.push_back(output_type_ptr_ptr);
            }
        }

        // inputs to the stage, with the additional counters added on
        std::vector<MType *> extern_wrapper_param_types;
        extern_wrapper_param_types.push_back(input_type_ptr_ptr);
        // the number of data elements coming in
        extern_wrapper_param_types.push_back(MPrimType::get_long_type());
        // the total size of the arrays in the data elements coming in
        extern_wrapper_param_types.push_back(MPrimType::get_long_type());

        // return type of the stage, with the additional counters added on
        std::vector<MType *> extern_wrapper_return_types;
        // the actual data type passed back
        extern_wrapper_return_types.push_back(output_type_ptr_ptr);
        // the number of data elements going out
        extern_wrapper_return_types.push_back(MPrimType::get_long_type());
        // the total size of the arrays in the data going out
        extern_wrapper_return_types.push_back(MPrimType::get_long_type());
        MStructType *return_type_struct_ptr = new MStructType(mtype_struct, extern_wrapper_return_types);
        MPointerType *pointer_return_type = new MPointerType(return_type_struct_ptr);
        mtypes_to_delete.push_back(return_type_struct_ptr);
        mtypes_to_delete.push_back(pointer_return_type);

        MFunc *func = new MFunc(function_name, stage_name, extern_return_type, pointer_return_type,
                                extern_param_types, extern_wrapper_param_types, jit);

        set_function(func);

        loop = new ForLoop(jit, mfunction);
        stage_arg_loader = new StageArgLoaderIB();
        extern_arg_loader = new ExternArgLoaderIB();
        call = new ExternCallIB();
        if (is_fixed_size) {
            preallocator = new FixedPreallocatorIB();
        } else {
            preallocator = new MatchedPreallocatorIB();
        }
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

    virtual llvm::Value *compute_data_array_size();

    /**
     * Preallocate output space based on the type.
     */
    virtual void preallocate();

    /**
     * Do the final malloc for the output struct, save the outputs to it, return from the stage
     */
    virtual llvm::AllocaInst *finish_stage(llvm::AllocaInst *output_data_array_size);

    JIT *get_jit();

};

#endif //MATCHIT_STAGE_H
