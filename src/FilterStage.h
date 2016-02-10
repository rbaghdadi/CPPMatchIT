//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_FILTERBLOCK_H
#define MATCHIT_FILTERBLOCK_H

#include <string>
#include "./Stage.h"
#include "./CodegenUtils.h"
#include "./ForLoop.h"

template <typename I>
class FilterStage : public Stage {

private:

    bool (*filter)(I);

public:

    FilterStage(bool (*filter)(I), std::string filter_name, JIT *jit, MType *param_type) :
            Stage(jit, mtype_of<I>(), mtype_bool, filter_name), filter(filter) {
        std::vector<MType *> param_types;
        param_types.push_back(param_type);
        MType *stage_return_type = new MPointerType(new WrapperOutputType(param_type));
        MFunc *func = new MFunc(function_name, "FilterStage", create_type<bool>(), stage_return_type,
                                param_types, jit);
        set_function(func);
    }

    ~FilterStage() {}

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternArgLoaderIB *eal,
                                ExternCallIB *ec, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {

        // build the body
        eal->set_mfunction(mfunction);
        eal->set_loop_idx_alloc(loop_idx);
        eal->set_wrapper_input_arg_alloc(args[0]);
        eal->codegen(jit, false);
        jit->get_builder().CreateBr(ec->get_basic_block());

        // create the call
        std::vector<llvm::AllocaInst *> sliced;
        sliced.push_back(eal->get_extern_input_arg_alloc());
        ec->set_mfunction(mfunction);
        ec->set_extern_arg_allocs(sliced);
        ec->codegen(jit, false);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(ec->get_extern_call_result_alloc());//get_secondary_extern_call_result_alloc());
        std::vector<llvm::Value *> bool_gep_idx;
        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(load, CodegenUtils::get_i1(0));
        ec->set_secondary_extern_call_result_alloc(eal->get_extern_input_arg_alloc());
        jit->get_builder().CreateCondBr(cmp, loop->get_for_loop_increment_basic_block()->get_basic_block(), branch_to);

    }

};

#endif //MATCHIT_FILTERBLOCK_H
