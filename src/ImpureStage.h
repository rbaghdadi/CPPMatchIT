//
// Created by Jessica Ray on 1/30/16.
//

#ifndef MATCHIT_IMPURESTAGE_H
#define MATCHIT_IMPURESTAGE_H

#include "./MType.h"
#include "./Stage.h"

class ImpureStage : public Stage {

private:

    ExternCallBasicBlock *extern_call2_basic_block;

public:

    ImpureStage(std::string stage_name, JIT *jit, MFunc *mfunction, mtype_code_t input_mtype_code,
                mtype_code_t output_mtype_code) : Stage(jit, input_mtype_code, output_mtype_code, stage_name) {
        set_function(mfunction);
        extern_call2_basic_block = new ExternCallBasicBlock();
    }

    ~ImpureStage() {
        delete extern_call2_basic_block;
    }

    void codegen() { }

    void postprocess(Stage *stage, llvm::BasicBlock *branch_from, llvm::BasicBlock *branch_into) { }

    // TODO can merge this into the regular extern call block if you codegen immediately (if you overrwrite it)
    ExternCallBasicBlock *get_extern_call2_basic_block() {
        return extern_call2_basic_block;
    }

};


#endif //MATCHIT_IMPURESTAGE_H
