//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMBLOCK_H
#define MATCHIT_TRANSFORMBLOCK_H

#include "CodegenUtils.h"
#include "./Block.h"
#include "./CodegenUtils.h"
#include "./MFunc.h"
#include "./MType.h"

template <typename I, typename O>
class TransformBlock : public Block {

private:

    // TODO this doesn't allow structs of structs
    std::vector<MType *> input_struct_fields;
    std::vector<MType *> output_struct_fields;
    O (*transform)(I);

    void codegen_postprocess(llvm::AllocaInst *alloc_ret_idx, llvm::AllocaInst *alloc_ret, llvm::Value *call_res) {
        // save the result of the function call
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

public:

    TransformBlock(O (*transform)(I), std::string transform_name, JIT *jit, std::vector<MType *> input_struct_fields = std::vector<MType *>(),
                   std::vector<MType *> output_struct_fields = std::vector<MType *>()) :
            Block(jit, mtype_of<I>(), mtype_of<O>(), transform_name), transform(transform),
            input_struct_fields(input_struct_fields), output_struct_fields(output_struct_fields) {
    }

    void codegen() {
        // TODO what if input type is a struct (see ComparisonBlock)
        MType *ret_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if (output_type_code == mtype_ptr && !output_struct_fields.empty()) {
            ret_type = create_struct_reference_type(output_struct_fields);
        } else if (output_type_code != mtype_struct) {
            ret_type = create_type<O>();
        } else {
            ret_type = create_struct_type(output_struct_fields);
        }

        MType *arg_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if(input_type_code == mtype_ptr && !input_struct_fields.empty()) {
            arg_type = create_struct_reference_type(input_struct_fields);
        } else if (input_type_code != mtype_struct) {
            arg_type = create_type<I>();
        } else {
            arg_type = create_struct_type(input_struct_fields);
        }

        std::vector<MType *> arg_types;
        arg_types.push_back(arg_type);
        MFunc *func = new MFunc(function_name, "TransformBlock", ret_type, arg_types, jit);
        set_function(func);

        // build up the boilerplate parts of the block
        generic_codegen_tuple tup = codegen_init();

        // ignore error in clion
        // add on the Block specific processing
        codegen_postprocess(std::get<3>(tup), std::get<4>(tup), std::get<5>(tup));
        LLVMIR::branch(jit, std::get<0>(tup));

        // create the return after the loop
        codegen_loop_end_block(std::get<1>(tup), std::get<4>(tup), std::get<3>(tup));

        func->verify_wrapper();
    }

    ~TransformBlock() {}
};

#endif //MATCHIT_TRANSFORMBLOCK_H
