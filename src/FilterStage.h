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

    FilterStage(bool (*filter)(I), std::string filter_name, JIT *jit, std::vector<MType *> input_struct_fields = std::vector<MType *>()) :
            Stage(jit, mtype_of<I>(), mtype_bool, filter_name), filter(filter) {
        MType *arg_type = create_type<I>();
        std::vector<MType *> arg_types;
        arg_types.push_back(arg_type);
        MFunc *func = new MFunc(function_name, "FilterStage", create_type<bool>(), arg_types, jit);
        func->codegen_extern_proto();
        func->codegen_extern_wrapper_proto();
        set_function(func);
    }

    ~FilterStage() {}

    FilterStage(const FilterStage &that) : Stage(that) {
        filter = that.filter;
    }

    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb,
                                    ExternCallBasicBlock *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
        // build the body
        eibb->set_loop_idx(loop_idx);
        eibb->set_data(args[0]);
        eibb->codegen(jit, false);
        jit->get_builder().CreateBr(ecbb->get_basic_block());

        // create the call
        ecbb->set_extern_function(mfunction);
        std::vector<llvm::Value *> sliced;
        sliced.push_back(eibb->get_element());
        ecbb->set_extern_args(sliced);
        ecbb->codegen(jit, false);
        llvm::LoadInst *load = jit->get_builder().CreateLoad(ecbb->get_data_to_return());
        llvm::Value *cmp = jit->get_builder().CreateICmpEQ(load, llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), 0));
        ecbb->override_data_to_return(eibb->get_element());
        jit->get_builder().CreateCondBr(cmp, loop->get_for_loop_increment_basic_block()->get_basic_block(), branch_to);
    }
};

#endif //MATCHIT_FILTERBLOCK_H
