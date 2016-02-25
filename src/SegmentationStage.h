//
// Created by Jessica Ray on 2/3/16.
//

#ifndef MATCHIT_SEGMENTATIONSTAGE_H
#define MATCHIT_SEGMENTATIONSTAGE_H

#include "./Stage.h"

template <typename I, typename O>
class SegmentationStage : public Stage {

private:

    void (*segment)(const I*, O**);
    unsigned int segment_size;
    float overlap;

public:

    SegmentationStage(void (*segment)(const I*, O**), std::string segmentation_name, JIT *jit, MType *param_type,
                      MType *return_type, unsigned int segment_size, float overlap) :
            Stage(jit, "SegmentationStage", segmentation_name, param_type, return_type, MPrimType::get_void_type(),
                  true, segment_size),
            segment(segment), segment_size(segment_size), overlap(overlap) { }

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    bool is_segmentation() {
        return true;
    }

    llvm::Value *compute_num_segments() {
        llvm::Value *segment_size_float =
                llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), (float)segment_size);
        llvm::Value *overlap_float =
                llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), overlap);
        llvm::Value *num_prim_values_float =
                jit->get_builder().CreateUIToFP(jit->get_builder().CreateLoad(stage_arg_loader->get_data_array_size()),
                                                llvm::Type::getFloatTy(llvm::getGlobalContext()));
        llvm::Value *numerator =
                jit->get_builder().CreateFSub(num_prim_values_float, jit->get_builder().CreateFMul(segment_size_float,
                                                                                                   overlap_float));
        llvm::Value *denominator =
                jit->get_builder().CreateFSub(segment_size_float, jit->get_builder().CreateFMul(segment_size_float,
                                                                                                overlap_float));
        llvm::Value *num_segments =
                jit->get_builder().CreateSub(jit->get_builder().CreateFPToUI(jit->get_builder().CreateFDiv(numerator, denominator),
                                                                             llvm::Type::getInt64Ty(llvm::getGlobalContext())),
                                             CodegenUtils::get_i64(1));
        return num_segments;
    }

    llvm::Value *compute_preallocation_data_array_size() {
        return jit->get_builder().CreateMul(compute_num_segments(), CodegenUtils::get_i64(segment_size));
    }

    void handle_extern_output(llvm::AllocaInst *output_data_array_size) { }

    void finalize_data_array_size(llvm::AllocaInst *output_data_array_size) {
        llvm::Value *x = jit->get_builder().CreateMul(compute_num_segments(), CodegenUtils::get_i64(segment_size));
        jit->get_builder().CreateStore(x, output_data_array_size);
    }

    llvm::AllocaInst *compute_num_output_structs() {
        llvm::Value *num = compute_num_segments();
        llvm::AllocaInst *num_alloca = jit->get_builder().CreateAlloca(num->getType());
        jit->get_builder().CreateStore(num, num_alloca);
        return num_alloca;
    }

//    void codegen2() {
//        if (!codegen_done) {
//            init_codegen();
//
//            // need to calculate number of segments that will be created
//            // pass in the outer struct (ptr to ptr) instead of ptr because user needs to create a bunch of segments
//
//
//            // load the inputs to the wrapper function
//            WrapperArgLoaderIB wal;
//            wal.insert(mfunction->get_extern_wrapper());
//            wal.codegen(jit);
//            ExternArgLoaderIB eal;
//            eal.insert(mfunction->get_extern_wrapper());
//
//            ForLoop loop(jit, mfunction->get_extern_wrapper());
//            llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy",
//                                                               mfunction->get_extern_wrapper());
//            jit->get_builder().CreateBr(loop.get_counter_bb());
//
//            // loop counters
//            jit->get_builder().SetInsertPoint(loop.get_counter_bb());
//            llvm::AllocaInst *counters[2];
//            loop.codegen_counters(counters, 2);
//            llvm::AllocaInst *loop_idx = counters[0];
//            llvm::AllocaInst *loop_bound = counters[1];
//            jit->get_builder().CreateStore(
//                    jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 1]), loop_bound);
//
//            llvm::AllocaInst *num_prim_values_ctr = jit->get_builder().CreateAlloca(
//                    llvm::Type::getInt64Ty(llvm::getGlobalContext()));
//            jit->get_builder().CreateStore(
//                    jit->get_builder().CreateLoad(wal.get_args_alloc()[wal.get_args_alloc().size() - 2]),
//                    num_prim_values_ctr);
//            // preallocate space for the output
//            // get the output type underneath the pointer it is wrapper in
//            // compute the number of segments that will result
//            // this segmentation scheme assumes that users have already padded data correctly for segmenting.
//            // TODO document how this works. The user needs to adhere to my segmentation rules otherwise they may try to compute too many segments
//            llvm::Value *segment_size_float =
//                    llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), (float)segment_size);
//            llvm::Value *overlap_float =
//                    llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), overlap);
//            llvm::Value *num_prim_values_float =
//                    jit->get_builder().CreateUIToFP(jit->get_builder().CreateLoad(num_prim_values_ctr), llvm::Type::getFloatTy(llvm::getGlobalContext()));
//            llvm::Value *numerator =
//                    jit->get_builder().CreateFSub(num_prim_values_float, jit->get_builder().CreateFMul(segment_size_float,
//                                                                                                       overlap_float));
//            llvm::Value *denominator =
//                    jit->get_builder().CreateFSub(segment_size_float, jit->get_builder().CreateFMul(segment_size_float,
//                                                                                                    overlap_float));
//            llvm::Value *num_segments =
//                    jit->get_builder().CreateSub(jit->get_builder().CreateFPToUI(jit->get_builder().CreateFDiv(numerator, denominator),
//                                                                                 llvm::Type::getInt64Ty(llvm::getGlobalContext())), CodegenUtils::get_i64(1));
//            llvm::AllocaInst *space = mfunction->get_extern_param_types()[1]->get_underlying_types()[0]->get_underlying_types()[0]->
//                    preallocate_fixed_block(jit, num_segments,
//                                            jit->get_builder().CreateMul(num_segments, CodegenUtils::get_i64(segment_size)),
//                                            CodegenUtils::get_i64(segment_size), mfunction->get_extern_wrapper()); // the output element is the second argument to our extern function
//            jit->get_builder().CreateBr(loop.get_condition_bb());
//            // loop condition
//            loop.codegen_condition(loop_bound, loop_idx);
//            jit->get_builder().CreateCondBr(loop.get_condition()->get_loop_comparison(), eal.get_basic_block(), dummy);
//
//            // loop increment
//            loop.codegen_increment(loop_idx);
//            jit->get_builder().CreateBr(loop.get_condition_bb());
//
//            // loop body
//            // get the inputs to the extern function
//            eal.set_loop_idx_alloc(loop_idx);
//            eal.set_no_output_param(space);
//            eal.set_stage_input_arg_alloc(wal.get_args_alloc()[0]);
//            eal.set_segmentation_stage(); // yes, this is the segmentation stage
//            eal.codegen(jit);
//
//            // call the extern function
//            ExternCallIB ec;
//            ec.insert(mfunction->get_extern_wrapper());
//            ec.set_extern_function(mfunction->get_extern());
//            jit->get_builder().CreateBr(ec.get_basic_block());
//            ec.set_extern_arg_allocs(eal.get_extern_input_arg_allocs());
//            ec.codegen(jit);
//            jit->get_builder().CreateBr(loop.get_increment_bb());
//
//            // finish up the stage and allocate space for the final output struct
//            jit->get_builder().SetInsertPoint(dummy);
//            llvm::AllocaInst *wrapper_result = jit->get_builder().CreateAlloca(
//                    mfunction->get_extern_wrapper_return_type()->codegen_type());
//            // RetStruct *rs = (RetStruct*)malloc(sizeof(RetStruct)), RetStruct = { { i64, i64, float* }**, i64, i64 }, so sizeof = 24
//            llvm::Value *return_space = CodegenUtils::codegen_c_malloc64_and_cast(jit, 24,
//                                                                                  mfunction->get_extern_wrapper_return_type()->codegen_type());
//            jit->get_builder().CreateStore(return_space, wrapper_result);
//            llvm::LoadInst *temp_wrapper_result_load = jit->get_builder().CreateLoad(wrapper_result);
//
//            // store the preallocated space in the output struct (these gep idxs are for the different fields in the output struct)
//            // store all the processed structs in field one
//            llvm::Value *field_one = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 0);
//            llvm::Value *field_two = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 1);
//            llvm::Value *field_three = CodegenUtils::gep(jit, temp_wrapper_result_load, 0, 2);
//            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(space), field_one);
//            // store the number of primitive values across all the structs
//            llvm::Value *num_prim_values = jit->get_builder().CreateMul(CodegenUtils::get_i64(segment_size),
//                                                                        num_segments);
//            jit->get_builder().CreateStore(num_prim_values, field_two);
//            // store the number of structs being returned
//            jit->get_builder().CreateStore(jit->get_builder().CreateLoad(loop_idx), field_three);
//            jit->get_builder().CreateRet(jit->get_builder().CreateLoad(wrapper_result));
//            codegen_done = true;
//        }
//    }

};


#endif //MATCHIT_SEGMENTATIONSTAGE_H
