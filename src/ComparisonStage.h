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

    O (*compare)(I,I);
    // loop over the second input stream of data
    ForLoop *inner_loop;

public:

    ComparisonStage(void (*compare)(const I*, const I*, O*), std::string compare_name, JIT *jit, MType *param_type,
                    MType *return_type, bool is_fixed_size = false, unsigned int fixed_size = 0) :
            Stage(jit, "ComparisonStage", compare_name, param_type, return_type, MScalarType::get_void_type(),
                  is_fixed_size, fixed_size), compare(compare) {
        inner_loop = new ForLoop(jit, mfunction);
    }

    ~ComparisonStage() {
        delete inner_loop;
    }

    bool is_comparison() {
        return true;
    }

    std::vector<llvm::AllocaInst *> get_extern_arg_loader_idxs() {
        std::vector<llvm::AllocaInst *> idxs;
        idxs.push_back(loop->get_loop_idx());
        idxs.push_back(inner_loop->get_loop_idx());
        return idxs;
    }

    std::vector<llvm::AllocaInst *> get_extern_arg_loader_data() {
        std::vector<llvm::AllocaInst *> data;
        data.push_back(stage_arg_loader->get_data(0));
        data.push_back(stage_arg_loader->get_data(1));
        return data;
    }



//    ComparisonStage(O (*compare)(I,I), std::string compare_name, JIT *jit, MType *param_type, MType *return_type) : Stage(jit, mtype_of<I>(), mtype_of<O>(),
//                                                                                                                          compare_name), compare(compare) {
////        MType *return_type = create_type<O>();
////        MType *param_type = create_type<I>();
//        std::vector<MType *> param_types;
//        param_types.push_back(param_type);
//        param_types.push_back(param_type);
//        MType *stage_return_type = new MPointerType(new WrapperOutputType(return_type));
//        MFunc *func = new MFunc(function_name, "ComparisonStage", new MPointerType(return_type), stage_return_type,
//                                param_types, jit);
//        set_function(func);
//    }
//
//    ~ComparisonStage() { }
//
//    void init_codegen() {
//        mfunction->codegen_extern_proto();
//        mfunction->codegen_extern_wrapper_proto();
//    }
//
//    void stage_specific_codegen(std::vector<llvm::AllocaInst *> args, ExternArgLoaderIB *eal,
//                                ExternCallIB *ec, llvm::BasicBlock *branch_to, llvm::AllocaInst *loop_idx) {
//        inner_loop = new ForLoop(jit);
//        inner_loop->set_mfunction(mfunction);
//
//        // build the body
//        // outer loop
//        eal->set_mfunction(mfunction);
//        eal->set_loop_idx_alloc(loop_idx);
//        eal->set_wrapper_input_arg_alloc(args[0]);
//        eal->codegen(jit, false);
//        jit->get_builder().CreateBr(inner_loop->get_loop_counter_basic_block()->get_basic_block());
//
//        // alright, get the second loop going
//        ExternArgLoaderIB *inner_eal = new ExternArgLoaderIB();
//        inner_loop->get_for_loop_condition_basic_block()->force_insert(mfunction); // we need this to get the bb initialized
//        inner_loop->set_max_loop_bound(last(args));
//        inner_loop->set_branch_to_after_counter(inner_loop->get_for_loop_condition_basic_block()->get_basic_block()); // go to the inner loop
//        inner_loop->set_branch_to_true_condition(inner_eal->get_basic_block()); // get next input of inner
//        inner_loop->set_branch_to_false_condition(loop->get_for_loop_increment_basic_block()->get_basic_block()); // get next input of outer
//        inner_loop->codegen();
//
//        // now get data for the second input
//        inner_eal->set_mfunction(mfunction);
//        inner_eal->set_wrapper_input_arg_alloc(args[1]);
//        inner_eal->set_loop_idx_alloc(inner_loop->get_loop_counter_basic_block()->get_loop_idx_alloc());
//        inner_eal->codegen(jit, false);
//        jit->get_builder().CreateBr(ec->get_basic_block());
//
//        // now create the call to the extern function
//        // the first input comes from the outer loop and the second comes from the inner loop
//        std::vector<llvm::AllocaInst *> elements;
//        elements.push_back(eal->get_extern_input_arg_alloc());
//        elements.push_back(inner_eal->get_extern_input_arg_alloc());
//        ec->set_mfunction(mfunction);
//        ec->set_extern_arg_allocs(elements);
//        ec->codegen(jit, false);
//        jit->get_builder().CreateBr(branch_to);
//
//        // cleanup
//        delete inner_eal;
//    }
//
//    llvm::BasicBlock *branch_to_after_store() {
//        return inner_loop->get_for_loop_increment_basic_block()->get_basic_block();
//    }

};


#endif //MATCHIT_COMPARISONBLOCK_H

