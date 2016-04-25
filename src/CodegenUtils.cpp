//
// Created by Jessica Ray on 1/28/16.
//

#include "./CodegenUtils.h"
#include "./ForLoop.h"

namespace Codegen {

llvm::ConstantFP *as_float(float x) {
    return llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(x));
}

llvm::ConstantInt *as_i1(bool x) {
    return llvm::ConstantInt::get(llvm_int1, x);
}

llvm::ConstantInt *as_i8(char x) {
    return llvm::ConstantInt::get(llvm_int8, x);
}

llvm::ConstantInt *as_i16(short x) {
    return llvm::ConstantInt::get(llvm_int16, x);
}

llvm::ConstantInt *as_i32(int x) {
    return llvm::ConstantInt::get(llvm_int32, x);
}

llvm::ConstantInt *as_i64(long x) {
    return llvm::ConstantInt::get(llvm_int64, x);
}


llvm::Value *gep_i64_i32(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx) {
    std::vector<llvm::Value *> idxs;
    idxs.push_back(as_i64(ptr_idx));
    idxs.push_back(as_i32(struct_idx));
    llvm::Value *gep = jit->get_builder().CreateInBoundsGEP(gep_this, idxs);
    return gep;
}

llvm::Value *codegen_llvm_gep(JIT *jit, llvm::Value *gep_this, std::vector<llvm::Value *> gep_idxs) {
    llvm::Value *gep = jit->get_builder().CreateInBoundsGEP(gep_this, gep_idxs);
    return gep;
}

// load up the input data
// give the inputs arguments names so we can actually use them
// the last argument is the size of the array of data being fed in. It is NOT user created
std::vector<llvm::AllocaInst *> load_stage_input_params(JIT *jit, llvm::Function *stage) {
    unsigned ctr = 0;
    std::vector<llvm::AllocaInst *> params;
    for (llvm::Function::arg_iterator iter = stage->arg_begin(); iter != stage->arg_end(); iter++) {
        iter->setName("x_" + std::to_string(ctr++));
        llvm::AllocaInst *param = codegen_llvm_alloca(jit, iter->getType(), 8);
        codegen_llvm_store(jit, iter, param, 8);
        params.push_back(param);
    }
    return params;
}

// load a single input argument from the stage inputs
std::vector<llvm::AllocaInst *> load_user_function_input_param(JIT *jit,
                                                               std::vector<llvm::AllocaInst *> stage_input_param_alloc,
                                                               llvm::AllocaInst *preallocated_output_space,
                                                               std::vector<llvm::AllocaInst *> loop_idx,
                                                               bool is_segmentation_stage, bool is_filter_stage,
                                                               bool has_output_param, llvm::AllocaInst *output_idx) {
    std::vector<llvm::AllocaInst *> arg_types;
    // get all of the SetElements
    llvm::LoadInst *input_set_elements = codegen_llvm_load(jit, stage_input_param_alloc[0], 8); // TODO why don't I just pass in everything so I can index these as 0 and 2. Seems a little more natural
    llvm::LoadInst *output_set_elements;
    if (!is_filter_stage) {
        output_set_elements = codegen_llvm_load(jit, stage_input_param_alloc[stage_input_param_alloc.size() - 1], 8);
    }

    // extract the input/output Element corresponding to this loop iteration
    llvm::LoadInst *loop_idx_load = codegen_llvm_load(jit, loop_idx[0], 4);
    std::vector<llvm::Value *> element_idxs;
    element_idxs.push_back(loop_idx_load);

    llvm::Value *input_set_element_gep = codegen_llvm_gep(jit, input_set_elements, element_idxs);
    llvm::LoadInst *input_set_element_load = codegen_llvm_load(jit, input_set_element_gep, 8);
    llvm::AllocaInst *input_set_element_alloc = codegen_llvm_alloca(jit, input_set_element_load->getType(), 8);
    codegen_llvm_store(jit, input_set_element_load, input_set_element_alloc, 8);
    arg_types.push_back(input_set_element_alloc);

    // TODO clean up
    if (loop_idx.size() == 3) { // this is a comparison stage with 3 indices
        llvm::LoadInst *loop_idx_load = codegen_llvm_load(jit, loop_idx[1], 4);
        std::vector<llvm::Value *> element_idxs;
        element_idxs.push_back(loop_idx_load);

        // input Element
        llvm::LoadInst *input_set_elements = codegen_llvm_load(jit, stage_input_param_alloc[1], 8);
        llvm::Value *input_set_element_gep = codegen_llvm_gep(jit, input_set_elements, element_idxs);
        llvm::LoadInst *input_set_element_load = codegen_llvm_load(jit, input_set_element_gep, 8);
        llvm::AllocaInst *input_set_element_alloc = codegen_llvm_alloca(jit, input_set_element_load->getType(), 8);
        codegen_llvm_store(jit, input_set_element_load, input_set_element_alloc, 8);
        arg_types.push_back(input_set_element_alloc);
    }

    // output Element
    if (!is_filter_stage) { // a filter only gets inputs. outputs are implicitly handled
        std::vector<llvm::Value *> element_idxs;
        if (!is_segmentation_stage) {
            // TODO having loop_idxs mean separate things to separate stages is not good.
            // For a transformation stage, 0 is the loop idx for the input and 1 is the loop idx for the output (they are the same)
            // For a comparison, 0 is the loop idx for the first input and 1 is the loop idx for the second input.
            llvm::LoadInst *loop_idx_load = codegen_llvm_load(jit, loop_idx[loop_idx.size() - 1], 4); // TODO this needs to be handled better--assumes only 2 max SetElements in signature
            element_idxs.push_back(loop_idx_load);
        } else {
            llvm::LoadInst *output_idx_load = codegen_llvm_load(jit, output_idx, 4);
            element_idxs.push_back(output_idx_load);
        }
        llvm::Value *output_set_element_gep = codegen_llvm_gep(jit, output_set_elements, element_idxs); // *
        if (!is_segmentation_stage) {
            llvm::LoadInst *output_set_element_load = codegen_llvm_load(jit, output_set_element_gep, 8);
            llvm::AllocaInst *output_set_element_alloc = codegen_llvm_alloca(jit, output_set_element_load->getType(), 8); // ** alloc
            codegen_llvm_store(jit, output_set_element_load, output_set_element_alloc, 8);
            arg_types.push_back(output_set_element_alloc);
        } else {
            llvm::AllocaInst *gep_alloc = codegen_llvm_alloca(jit, output_set_element_gep->getType(), 8);
            codegen_llvm_store(jit, output_set_element_gep, gep_alloc, 8);
            arg_types.push_back(gep_alloc);
        }
    }
    return arg_types;
}

void increment_i32(JIT *jit, llvm::AllocaInst *i32_val, int step_size) {
    llvm::LoadInst *load = codegen_llvm_load(jit, i32_val, 4);
    llvm::Value *add = codegen_llvm_add(jit, load, llvm::ConstantInt::get(llvm_int32, step_size));
    codegen_llvm_store(jit, add, i32_val, 4);
}

llvm::Value *create_loop_condition_check(JIT *jit, llvm::AllocaInst *loop_idx_alloc, llvm::AllocaInst *max_loop_bound) {
    llvm::LoadInst *load_idx = codegen_llvm_load(jit, loop_idx_alloc, 4);
    llvm::LoadInst *load_bound = codegen_llvm_load(jit, max_loop_bound, 4);
    return jit->get_builder().CreateICmpSLT(load_idx, load_bound);
}

// create a call to a user function
llvm::AllocaInst *create_user_function_call(JIT *jit, llvm::Function *user_function,
                                            std::vector<llvm::AllocaInst *> user_function_param_allocs) {
    std::vector<llvm::Value *> loaded_params;
    for (std::vector<llvm::AllocaInst *>::iterator iter = user_function_param_allocs.begin();
         iter != user_function_param_allocs.end(); iter++) {
        llvm::LoadInst *param = codegen_llvm_load(jit, *iter, 8);
        loaded_params.push_back(param);
    }

    llvm::CallInst *user_function_call_result = jit->get_builder().CreateCall(user_function, loaded_params);

    if (user_function_call_result->getType()->isVoidTy()) {
        return nullptr;
    } else {
        llvm::AllocaInst *user_function_call_alloc = codegen_llvm_alloca(jit, user_function_call_result->getType(), 8);
        codegen_llvm_store(jit, user_function_call_result, user_function_call_alloc, 8);
        return user_function_call_alloc;
    }
}

llvm::Value *codegen_llvm_ceil(JIT *jit, llvm::Value *ceil_me) {
    llvm::Function *llvm_ceil = jit->get_module()->getFunction("llvm.ceil.f32");
    assert(llvm_ceil);
    std::vector<llvm::Value *> ceil_args;
    ceil_args.push_back(ceil_me);
    return jit->get_builder().CreateCall(llvm_ceil, ceil_args);
}

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src, llvm::Value *bytes_to_copy) {
    llvm::Function *llvm_memcpy = jit->get_module()->getFunction("llvm.memcpy.p0i8.p0i8.i32");
    assert(llvm_memcpy);
    std::vector<llvm::Value *> memcpy_args;
    memcpy_args.push_back(jit->get_builder().CreateBitCast(dest, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    memcpy_args.push_back(jit->get_builder().CreateBitCast(src, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    memcpy_args.push_back(bytes_to_copy);
    memcpy_args.push_back(llvm::ConstantInt::get(llvm_int32, 1));
    memcpy_args.push_back(llvm::ConstantInt::get(llvm_int1, false));
    jit->get_builder().CreateCall(llvm_memcpy, memcpy_args);
}

void codegen_fprintf_int(JIT *jit, llvm::Value *the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("c_fprintf");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    llvm::Value *casted = jit->get_builder().CreateTruncOrBitCast(the_int, llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    print_args.push_back(casted);
    jit->get_builder().CreateCall(c_fprintf, print_args);
}

void codegen_fprintf_int(JIT *jit, int the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("c_fprintf");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(llvm::ConstantInt::get(llvm_int32, the_int));
    jit->get_builder().CreateCall(c_fprintf, print_args);
}

void codegen_fprintf_float(JIT *jit, llvm::Value *the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("c_fprintf_float");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(the_int);
    jit->get_builder().CreateCall(c_fprintf, print_args);
}

void codegen_fprintf_float(JIT *jit, float the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("c_fprintf_float");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(llvm::ConstantInt::get(llvm_float, the_int));
    jit->get_builder().CreateCall(c_fprintf, print_args);
}

llvm::Value *codegen_mmalloc(JIT *jit, size_t size) {
    llvm::Function *mmalloc = jit->get_module()->getFunction("mmalloc");
    assert(mmalloc);
    std::vector<llvm::Value *> mmalloc_args;
    mmalloc_args.push_back(llvm::ConstantInt::get(llvm_int32, size));
    return jit->get_builder().CreateCall(mmalloc, mmalloc_args);
}

llvm::Value *codegen_mmalloc(JIT *jit, llvm::Value *size) {
    llvm::Function *mmalloc = jit->get_module()->getFunction("mmalloc");
    assert(mmalloc);
    std::vector<llvm::Value *> mmalloc_args;
    mmalloc_args.push_back(size);
    return jit->get_builder().CreateCall(mmalloc, mmalloc_args);
}

void codegen_mfree(JIT *jit, llvm::LoadInst *structure) {
    llvm::Function *mfree = jit->get_module()->getFunction("mfree");
    assert(mfree);
    std::vector<llvm::Value *> mfree_args;
    mfree_args.push_back(jit->get_builder().CreateBitCast(structure, llvm_int8Ptr));
    jit->get_builder().CreateCall(mfree, mfree_args);
}

void codegen_mdelete(JIT *jit, llvm::LoadInst *structure) {
    llvm::Function *mdelete = jit->get_module()->getFunction("mdelete");
    assert(mdelete);
    std::vector<llvm::Value *> mdelete_args;
    mdelete_args.push_back(jit->get_builder().CreateBitCast(structure, llvm_int8Ptr));
    jit->get_builder().CreateCall(mdelete, mdelete_args);
}

llvm::Value *codegen_mmalloc_and_cast(JIT *jit, size_t size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_mmalloc(jit, size), cast_to);
}

llvm::Value *codegen_mmalloc_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_mmalloc(jit, size), cast_to);
}

// loaded_structure is the original data that was malloc'd
llvm::Value *codegen_mrealloc(JIT *jit, llvm::LoadInst *structure, llvm::Value *size) {
    llvm::Function *mrealloc = jit->get_module()->getFunction("mrealloc");
    return codegen_mrealloc(jit, mrealloc, structure, size);
}

llvm::Value *codegen_mrealloc(JIT *jit, llvm::Function *mrealloc, llvm::LoadInst *structure, llvm::Value *size) {
    assert(mrealloc);
    std::vector<llvm::Value *> mrealloc_args;
    mrealloc_args.push_back(jit->get_builder().CreateBitCast(structure, llvm_int8Ptr));
    mrealloc_args.push_back(size);
    return jit->get_builder().CreateCall(mrealloc, mrealloc_args);
}

llvm::Value *codegen_mrealloc_and_cast(JIT *jit, llvm::LoadInst *structure, llvm::Value *size,
                                       llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_mrealloc(jit, structure, size), cast_to);
}

llvm::AllocaInst *codegen_llvm_alloca(JIT *jit, llvm::Type *type, unsigned int alignment, std::string name) {
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(type, nullptr, name);
    alloc->setAlignment(alignment);
    return alloc;
}

llvm::StoreInst *codegen_llvm_store(JIT *jit, llvm::Value *src, llvm::Value *dest, unsigned int alignment) {
    llvm::StoreInst *store = jit->get_builder().CreateStore(src, dest);
    store->setAlignment(alignment);
    return store;
}

llvm::LoadInst *codegen_llvm_load(JIT *jit, llvm::Value *src, unsigned int alignment) {
    llvm::LoadInst *load = jit->get_builder().CreateLoad(src);
    load->setAlignment(alignment);
    return load;
}

llvm::Type *codegen_llvm_ptr(JIT *jit, llvm::Type *element_type) {
    return llvm::PointerType::get(element_type, 0);
}

llvm::Value *codegen_llvm_mul(JIT *jit, llvm::Value *left, llvm::Value *right) {
    return jit->get_builder().CreateMul(left, right);
}

llvm::Value *codegen_llvm_add(JIT *jit, llvm::Value *left, llvm::Value *right) {
    return jit->get_builder().CreateAdd(left, right);
}

}