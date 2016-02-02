//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_COMPARISONBLOCK_H
#define MATCHIT_COMPARISONBLOCK_H

#include <string>
#include "llvm/IR/Verifier.h"
#include "./Stage.h"
#include "./MType.h"
#include "./ForLoop.h"
#include "./Utils.h"

template<class I, class O>
class ComparisonStage : public Stage {

private:

    // TODO this doesn't allow structs of structs
    std::vector<MType *> input_struct_fields;
    std::vector<MType *> output_struct_fields;
    O (*compare)(I,I);
    ForLoop *inner_loop;
//    LoopCounterBasicBlock *loop_counter_basic_block_inner;
//    ForLoopConditionBasicBlock *for_loop_condition_basic_block_inner;
//    ForLoopIncrementBasicBlock *for_loop_increment_basic_block_inner;
//    ExternInitBasicBlock *extern_init_basic_block_inner;

public:

    ComparisonStage(O (*compare)(I,I), std::string compare_name, JIT *jit, std::vector<MType *> input_struct_fields = std::vector<MType *>(),
                    std::vector<MType *> output_struct_fields = std::vector<MType *>()) :
            Stage(jit, mtype_of<I>(), mtype_of<O>(), compare_name), input_struct_fields(input_struct_fields),
            output_struct_fields(output_struct_fields), compare(compare) {
//        loop_counter_basic_block_inner = new LoopCounterBasicBlock();
//        for_loop_condition_basic_block_inner = new ForLoopConditionBasicBlock();
//        for_loop_increment_basic_block_inner = new ForLoopIncrementBasicBlock();
//        extern_init_basic_block_inner = new ExternInitBasicBlock();

        MType *ret_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if (output_mtype_code == mtype_ptr && !output_struct_fields.empty()) {
            ret_type = create_struct_reference_type(output_struct_fields);
        } else if (output_mtype_code != mtype_struct) {
            ret_type = create_type<O>();
        } else {
            ret_type = create_struct_type(output_struct_fields);
        }

        MType *arg_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if(input_mtype_code == mtype_ptr && !input_struct_fields.empty()) {
            arg_type = create_struct_reference_type(input_struct_fields);
        } else if (input_mtype_code != mtype_struct) {
            arg_type = create_type<I>();
        } else {
            arg_type = create_struct_type(input_struct_fields);
        }

        std::vector<MType *> arg_types;
        arg_types.push_back(arg_type);
        arg_types.push_back(arg_type);
        MFunc *func = new MFunc(function_name, "ComparisonStage", ret_type, arg_types, jit);
        set_function(func);
        func->codegen_extern_proto();
        func->codegen_extern_wrapper_proto();
    }

    ~ComparisonStage() {
//        if (inner_loop) {
//            delete inner_loop;
//        }
    }

    ComparisonStage(const ComparisonStage &that) : Stage(that) {
        input_struct_fields = std::vector<MType *>(that.input_struct_fields);
        output_struct_fields = std::vector<MType *>(that.output_struct_fields);
        compare = that.compare;
//        loop_counter_basic_block_inner = new LoopCounterBasicBlock();
//        for_loop_condition_basic_block_inner = new ForLoopConditionBasicBlock();
//        for_loop_increment_basic_block_inner = new ForLoopIncrementBasicBlock();
//        extern_init_basic_block_inner = new ExternInitBasicBlock();
    }

    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternInitBasicBlock *eibb,
                                    ExternCallBasicBlock *ecbb, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
        inner_loop = new ForLoop(jit, mfunction);

        // build the body
        eibb->set_loop_idx(loop_idx);//loop->get_loop_counter_basic_block()->get_loop_idx());
        eibb->set_data(args[0]);
        eibb->codegen(jit);
        jit->get_builder().CreateBr(inner_loop->get_loop_counter_basic_block()->get_basic_block());

        // alright, get the second loop going
        ExternInitBasicBlock *inner_eibb = new ExternInitBasicBlock();
        inner_eibb->set_function(ecbb->get_mfunction());
        inner_loop->set_max_loop_bound(last(args));
        inner_loop->set_branch_to_after_counter(inner_loop->get_for_loop_condition_basic_block()->get_basic_block());
        inner_loop->set_branch_to_true_condition(inner_eibb->get_basic_block());
        inner_loop->set_branch_to_false_condition(loop->get_for_loop_increment_basic_block()->get_basic_block());
        inner_loop->codegen();

        // now get data for the second input
        inner_eibb->set_data(args[1]);
        inner_eibb->set_loop_idx(inner_loop->get_loop_counter_basic_block()->get_loop_idx());
        inner_eibb->codegen(jit);
        jit->get_builder().CreateBr(ecbb->get_basic_block());

        // now create the call to the extern function
        std::vector<llvm::Value *> elements;
        elements.push_back(eibb->get_element());
        elements.push_back(inner_eibb->get_element());
        ecbb->set_extern_function(mfunction);
        ecbb->set_extern_args(elements);
        ecbb->codegen(jit);
        jit->get_builder().CreateBr(branch_to);

        // cleanup
//        delete inner_eibb;
    }

    virtual llvm::BasicBlock *branch_to_after_store() {
        return inner_loop->get_for_loop_increment_basic_block()->get_basic_block();
    }

};


#endif //MATCHIT_COMPARISONBLOCK_H

