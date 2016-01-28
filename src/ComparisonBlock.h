//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_COMPARISONBLOCK_H
#define MATCHIT_COMPARISONBLOCK_H

#include <string>
#include "llvm/IR/Verifier.h"
#include "./Block.h"


// collection of stuff to pass around
typedef struct {

    llvm::BasicBlock *outer_for_inc_block;
    llvm::BasicBlock *inner_for_inc_block;
    llvm::BasicBlock *outer_for_end_block;
    llvm::AllocaInst *outer_alloc_loop_idx;
    llvm::AllocaInst *inner_alloc_loop_idx;
    llvm::AllocaInst *stack_alloc_ret_idx;
    llvm::AllocaInst *stack_alloc_ret_val;
    llvm::Value *extern_call_res;

} CompBlockVals;

template<class I, class O>
class ComparisonBlock: public Block {

private:

    // TODO this doesn't allow structs of structs
    std::vector<MType *> input_struct_fields;
    std::vector<MType *> output_struct_fields;
    O (*compare)(I,I);

    void codegen_postprocess(llvm::AllocaInst *alloc_ret_idx, llvm::AllocaInst *alloc_ret, llvm::Value *call_res) {
        llvm::AllocaInst *call_alloc = jit->get_builder().CreateAlloca(call_res->getType());
        jit->get_builder().CreateStore(call_res, call_alloc);

        // load the current ret idx to figure out where to store the output of the transform extern function
        llvm::LoadInst *ret_idx = jit->get_builder().CreateLoad(alloc_ret_idx);
        ret_idx->setAlignment(8);

        // load the return struct
        llvm::LoadInst *loaded_field = LLVMIR::codegen_load_struct_field(jit, alloc_ret, 0);
        std::vector<llvm::Value *> gep_element_idx;
        gep_element_idx.push_back(ret_idx);
        llvm::Value *gep_element = jit->get_builder().CreateInBoundsGEP(loaded_field, gep_element_idx);
        // load the actual index into the return data that we want to eventually store data into
        llvm::LoadInst *address = jit->get_builder().CreateLoad(gep_element);

        // now we have the pointer to the address where we should store the computed value from the extern call
        // but first, we must allocate space for that
        // TODO this assumes that the underlying element type is some type of array or pointer, which it might not always be, so we need some type of check
        llvm::Value *bitcast = LLVMIR::codegen_c_malloc_and_cast(jit, (mfunction->get_ret_type()->get_bits() / 8) * 50,
                                                                 mfunction->get_ret_type()->codegen());

        // load the return struct again
        loaded_field = LLVMIR::codegen_load_struct_field(jit, alloc_ret, 0);

        // store the malloc'd space in the return struct
        LLVMIR::codegen_store_malloc_in_struct(jit, loaded_field, bitcast, alloc_ret_idx);

        // now we get that same address again :)
        ret_idx = jit->get_builder().CreateLoad(alloc_ret_idx);
        ret_idx->setAlignment(8);
        loaded_field = LLVMIR::codegen_load_struct_field(jit, alloc_ret, 0);
        gep_element = jit->get_builder().CreateInBoundsGEP(loaded_field, gep_element_idx);
        address = jit->get_builder().CreateLoad(gep_element);

        // copy over the data computed from the extern transform call
        llvm::LoadInst *transformed_data = jit->get_builder().CreateLoad(call_alloc);
        LLVMIR::codegen_llvm_memcpy(jit, address, transformed_data);

        // increment the return index
        LLVMIR::codegen_ret_idx_inc(jit, alloc_ret_idx);
    }




    CompBlockVals *codegen_compblock_init() {
        // create function prototypes
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();

        // build up the block
        llvm::BasicBlock *loop_idx_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.idx", mfunction->get_extern_wrapper());
        llvm::BasicBlock *ret_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ret", mfunction->get_extern_wrapper());
        llvm::BasicBlock *arg_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "args", mfunction->get_extern_wrapper());

        // comparison requires a double loop
        llvm::BasicBlock *outer_for_cond_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "outer.for.cond", mfunction->get_extern_wrapper());
        llvm::BasicBlock *outer_for_inc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "outer.for.inc", mfunction->get_extern_wrapper());
        llvm::BasicBlock *outer_for_body_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "outer.for.body", mfunction->get_extern_wrapper());
        llvm::BasicBlock *outer_for_end_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "outer.for.end", mfunction->get_extern_wrapper());

        llvm::BasicBlock *inner_for_cond_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "inner.for.cond", mfunction->get_extern_wrapper());
        llvm::BasicBlock *inner_for_inc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "inner.for.inc", mfunction->get_extern_wrapper());
        llvm::BasicBlock *inner_for_body_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "inner.for.body", mfunction->get_extern_wrapper());
//        llvm::BasicBlock *inner_for_end_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "inner.for.end", mfunction->get_extern_wrapper());

        // initialize the loop counters
        jit->get_builder().SetInsertPoint(loop_idx_block);
        llvm::AllocaInst *outer_alloc_loop_idx = LLVMIR::init_loop_idx(jit, 0);
        llvm::AllocaInst *inner_alloc_loop_idx = LLVMIR::init_loop_idx(jit, 0);
        llvm::AllocaInst *loop_max_bound = codegen_init_loop_max_bound();
        llvm::AllocaInst *alloc_max_return_size = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
        llvm::LoadInst *loaded_max_bound = jit->get_builder().CreateLoad(loop_max_bound);
        llvm::Value *max_return_size = jit->get_builder().CreateMul(loaded_max_bound, loaded_max_bound);
        llvm::StoreInst *store_max = jit->get_builder().CreateStore(max_return_size, alloc_max_return_size);
        LLVMIR::branch(jit, ret_block);

        // initialize the return value structures/fields
        jit->get_builder().SetInsertPoint(ret_block);
        llvm::AllocaInst *stack_alloc_ret_val = codegen_init_ret_stack();
        llvm::Value *heap_bitcast_ret_struct = codegen_init_ret_heap();
        llvm::AllocaInst *stack_alloc_ret_idx = codegen_init_loop_ret_idx();

        // add the pointer to heap memory to our stack location
        // struct
        llvm::StoreInst *heap_to_stack = jit->get_builder().CreateStore(heap_bitcast_ret_struct, stack_alloc_ret_val);
        // field in struct
        llvm::Value *heap_bitcast_ret_data = codegen_init_ret_data_heap(alloc_max_return_size);
        llvm::Value *loaded_stack_struct = jit->get_builder().CreateLoad(stack_alloc_ret_val);
        std::vector<llvm::Value *> gep_struct_idxs;
        gep_struct_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
        gep_struct_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
        llvm::Value *gep_struct = jit->get_builder().CreateInBoundsGEP(loaded_stack_struct, gep_struct_idxs);
        llvm::StoreInst *heap_to_stack_ret = jit->get_builder().CreateStore(heap_bitcast_ret_data, gep_struct);
        LLVMIR::branch(jit, arg_block);

        // initialize the arguments to the wrapper function
        jit->get_builder().SetInsertPoint(arg_block);
        std::vector<llvm::AllocaInst *> args = codegen_init_args();
        LLVMIR::branch(jit, outer_for_cond_block);

        // create the loop index increment
        // outer
        jit->get_builder().SetInsertPoint(outer_for_inc_block);
        LLVMIR::build_loop_inc_block(jit, outer_alloc_loop_idx, 0, 1);
        LLVMIR::branch(jit, outer_for_cond_block);
        // inner
        jit->get_builder().SetInsertPoint(inner_for_inc_block);
        LLVMIR::build_loop_inc_block(jit, inner_alloc_loop_idx, 0, 1);
        LLVMIR::branch(jit, inner_for_cond_block);

        // create the loop index condition comparison
        // outer
        jit->get_builder().SetInsertPoint(outer_for_cond_block);
        llvm::Value *outer_condition = LLVMIR::build_loop_cond_block(jit, outer_alloc_loop_idx, loop_max_bound);
        LLVMIR::branch_cond(jit, outer_condition, outer_for_body_block, outer_for_end_block);
        // inner
        jit->get_builder().SetInsertPoint(inner_for_cond_block);
        llvm::Value *inner_condition = LLVMIR::build_loop_cond_block(jit, inner_alloc_loop_idx, loop_max_bound);
        LLVMIR::branch_cond(jit, inner_condition, inner_for_body_block, outer_for_inc_block);

        // create the (initial) body of the loop
        // outer
        jit->get_builder().SetInsertPoint(outer_for_body_block);
        // in the outer body, we reset to 0 and enter the inner body
        llvm::StoreInst *reset_to_zero = jit->get_builder().CreateStore(LLVMIR::zero64, inner_alloc_loop_idx);
        LLVMIR::branch(jit, inner_for_body_block);
        // inner
        jit->get_builder().SetInsertPoint(inner_for_body_block);
        llvm::Value *call_res = codegen_inner_loop_body_block(outer_alloc_loop_idx, inner_alloc_loop_idx, args);

        // TODO change this, it is so painful to look at
//        generic_codegen_tuple codegen_ret(outer_for_inc_block, outer_for_end_block, outer_alloc_loop_idx, stack_alloc_ret_idx,
//                                          stack_alloc_ret_val, call_res, args[0]);

        CompBlockVals *cbv = (CompBlockVals*) malloc(sizeof(CompBlockVals));
        cbv->outer_for_inc_block = outer_for_inc_block;
        cbv->inner_for_inc_block = inner_for_inc_block;
        cbv->outer_for_end_block = outer_for_end_block;
        cbv->outer_alloc_loop_idx = outer_alloc_loop_idx;
        cbv->inner_alloc_loop_idx = inner_alloc_loop_idx;
        cbv->stack_alloc_ret_idx = stack_alloc_ret_idx;
        cbv->stack_alloc_ret_val = stack_alloc_ret_val;
        cbv->extern_call_res = call_res;

        return cbv;
    }


    llvm::Value *codegen_inner_loop_body_block(llvm::AllocaInst *alloc_outer_loop_idx, llvm::AllocaInst *alloc_inner_loop_idx, std::vector<llvm::AllocaInst *> args) {
        std::vector<llvm::Value*> extern_call_args;
        llvm::LoadInst *outer_loop_idx = jit->get_builder().CreateLoad(alloc_outer_loop_idx, "outer_loop_idx");
        llvm::LoadInst *inner_loop_idx = jit->get_builder().CreateLoad(alloc_inner_loop_idx, "inner_loop_idx");
        // load data that is passed to the extern function
        // get the first input
        llvm::LoadInst *outer_loaded_arg = jit->get_builder().CreateLoad(args[0]);
        std::vector<llvm::Value *> outer_gep_idxs;
        outer_gep_idxs.push_back(outer_loop_idx);
        llvm::Value *outer_gep = jit->get_builder().CreateInBoundsGEP(outer_loaded_arg, outer_gep_idxs, "mem_addr");
        llvm::LoadInst *outer_val = jit->get_builder().CreateLoad(outer_gep);
        extern_call_args.push_back(outer_val);
        // get the second input
        llvm::LoadInst *inner_loaded_arg = jit->get_builder().CreateLoad(args[1]);
        std::vector<llvm::Value *> inner_gep_idxs;
        inner_gep_idxs.push_back(inner_loop_idx);
        llvm::Value *inner_gep = jit->get_builder().CreateInBoundsGEP(inner_loaded_arg, inner_gep_idxs, "mem_addr");
        llvm::LoadInst *inner_val = jit->get_builder().CreateLoad(inner_gep);
        extern_call_args.push_back(inner_val);
        llvm::Value *extern_ret = jit->get_builder().CreateCall(mfunction->get_extern(), extern_call_args, "extern_ret");
        return extern_ret;
    }


public:

    ComparisonBlock(O (*compare)(I,I), std::string compare_name, JIT *jit, std::vector<MType *> input_struct_fields = std::vector<MType *>(),
                    std::vector<MType *> output_struct_fields = std::vector<MType *>()) : Block(jit, mtype_of<I>(), mtype_of<O>(), compare_name), compare(compare),
                                                                                          input_struct_fields(input_struct_fields), output_struct_fields(output_struct_fields) {
    }

    void codegen() {
        MType *ret_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if (output_type_code == mtype_ptr && !output_struct_fields.empty()) {
            ret_type = create_struct_reference_type(output_struct_fields);
        } else if (output_type_code != mtype_struct) {
            ret_type = create_type<O>();
        } else {
            ret_type = create_struct_type(output_struct_fields);
        }

        std::vector<MType *> arg_types;
        MType *arg_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if(input_type_code == mtype_ptr && !input_struct_fields.empty()) {
            arg_type = create_struct_reference_type(input_struct_fields);
        } else if (input_type_code != mtype_struct) {
            arg_type = create_type<I>();
        } else {
            arg_type = create_struct_type(input_struct_fields);
        }
        arg_types.push_back(arg_type);
        arg_types.push_back(arg_type);

        MFunc *func = new MFunc(function_name, "ComparisonBlock", ret_type, arg_types, jit);
        set_function(func);

        // build up the boilerplate parts of the block
        CompBlockVals *cbv = codegen_compblock_init();

        // for the comparison, we need to crete a second block3

        // ignore error in clion
        // add on the Block specific processing
        codegen_postprocess(cbv->stack_alloc_ret_idx, cbv->stack_alloc_ret_val, cbv->extern_call_res);
        LLVMIR::branch(jit, cbv->inner_for_inc_block);

//        codegen_postprocess(std::get<3>(tup), std::get<4>(tup), std::get<5>(tup));
//        LLVMIR::branch(jit, std::get<0>(tup)); // inner

        // create the return after the loop
//        codegen_loop_end_block(std::get<1>(tup), std::get<4>(tup), std::get<3>(tup));
        codegen_loop_end_block(cbv->outer_for_end_block, cbv->stack_alloc_ret_val, cbv->stack_alloc_ret_idx);


        func->verify_wrapper();
    }

    ~ComparisonBlock() {}

};


#endif //MATCHIT_COMPARISONBLOCK_H

