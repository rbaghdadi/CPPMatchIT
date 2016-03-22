//
// Created by Jessica Ray on 1/28/16.
//

#include "./CodegenUtils.h"
#include "./Structures.h"
#include "./MFunc.h"
#include "./InstructionBlock.h"
#include "./ForLoop.h"
#include "./MType.h"

namespace CodegenUtils {

llvm::ConstantInt *get_i1(int zero_or_one) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), zero_or_one);
}

llvm::ConstantInt *get_i32(int x) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), x);
}

llvm::ConstantInt *get_i64(long x) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), x);
}

llvm::Value *as_i32(JIT *jit, llvm::Value *i) {
    return jit->get_builder().CreateTruncOrBitCast(i, llvm::Type::getInt32Ty(llvm::getGlobalContext()));
}

// these are very specific geps
llvm::LoadInst *gep_and_load(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx) {
    llvm::LoadInst *load = jit->get_builder().CreateLoad(gep(jit, gep_this, ptr_idx, struct_idx));
    return load;
}

llvm::Value *gep(JIT *jit, llvm::Value *gep_this, long ptr_idx, int struct_idx) {
    std::vector<llvm::Value *> idxs;
    idxs.push_back(get_i64(ptr_idx));
    idxs.push_back(get_i32(struct_idx));
    llvm::Value *gep = jit->get_builder().CreateInBoundsGEP(gep_this, idxs);
    return gep;
}

// load up the input data
// give the inputs arguments names so we can actually use them
// the last argument is the size of the array of data being fed in. It is NOT user created
std::vector<llvm::AllocaInst *> load_wrapper_input_args(JIT *jit, llvm::Function *function) {
    llvm::Function *wrapper = function;
    unsigned ctr = 0;
    std::vector<llvm::AllocaInst *> alloc_args;
    for (llvm::Function::arg_iterator iter = wrapper->arg_begin(); iter != wrapper->arg_end(); iter++) {
        iter->setName("x_" + std::to_string(ctr++));
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(iter->getType());
        alloc->setAlignment(8);
        jit->get_builder().CreateStore(iter, alloc)->setAlignment(8);
        alloc_args.push_back(alloc);
    }
    return alloc_args;
}

// load a single input argument from the stage inputs
std::vector<llvm::AllocaInst *> load_user_function_input_arg(JIT *jit,
                                                             std::vector<llvm::AllocaInst *> stage_input_arg_alloc,
                                                             llvm::AllocaInst *preallocated_output_space,
                                                             std::vector<llvm::AllocaInst *> loop_idx, // two if comparison stage
                                                             bool is_segmentation_stage, bool has_output_param,
                                                             llvm::AllocaInst *output_idx) {
    std::vector<llvm::AllocaInst *> arg_types;
    // get all of the SetElements
    llvm::LoadInst *input_set_elements = jit->get_builder().CreateLoad(stage_input_arg_alloc[0]); // TODO why don't I just pass in everything so I can index these as 0 and 2. Seems a little more natural
    input_set_elements->setAlignment(8);
    llvm::LoadInst *output_set_elements = jit->get_builder().CreateLoad(stage_input_arg_alloc[1]);
    output_set_elements->setAlignment(8);

    // extract the input/output SetElement corresponding to this loop iteration
    llvm::LoadInst *loop_idx_load = jit->get_builder().CreateLoad(loop_idx[0]);
    loop_idx_load->setAlignment(8);
    std::vector<llvm::Value *> element_idxs;
    element_idxs.push_back(loop_idx_load);

    // input SetElement
    llvm::Value *input_set_element_gep = jit->get_builder().CreateInBoundsGEP(input_set_elements, element_idxs);
    llvm::LoadInst *input_set_element_load = jit->get_builder().CreateLoad(input_set_element_gep);
    input_set_element_load->setAlignment(8);
    llvm::AllocaInst *input_set_element_alloc = jit->get_builder().CreateAlloca(input_set_element_load->getType());
    input_set_element_alloc->setAlignment(8);
    jit->get_builder().CreateStore(input_set_element_load, input_set_element_alloc)->setAlignment(8);
    arg_types.push_back(input_set_element_alloc);

    // output SetElement
    llvm::Value *output_set_element_gep = jit->get_builder().CreateInBoundsGEP(output_set_elements, element_idxs);
    llvm::LoadInst *output_set_element_load = jit->get_builder().CreateLoad(output_set_element_gep);
    output_set_element_load->setAlignment(8);
    llvm::AllocaInst *output_set_element_alloc = jit->get_builder().CreateAlloca(output_set_element_load->getType());
    output_set_element_alloc->setAlignment(8);
    jit->get_builder().CreateStore(output_set_element_load, output_set_element_alloc)->setAlignment(8);
    arg_types.push_back(output_set_element_alloc);

    // store it
//    llvm::AllocaInst *extern_input_arg_alloc = jit->get_builder().CreateAlloca(input_set_element_load->getType());
//    extern_input_arg_alloc->setAlignment(8);
//    jit->get_builder().CreateStore(input_set_element_load, extern_input_arg_alloc)->setAlignment(8);
//    arg_types.push_back(extern_input_arg_alloc);



//    // load the full input array
//    int ctr = 0;
//    for (std::vector<llvm::AllocaInst *>::iterator loop_idx = loop_idxs.begin(); loop_idx != loop_idxs.end(); loop_idx++) {
//        llvm::LoadInst *input_arg_load = jit->get_builder().CreateLoad(stage_input_arg_alloc[ctr++]);
//        input_arg_load->setAlignment(8);
//        llvm::LoadInst *loop_idx_load = jit->get_builder().CreateLoad(*loop_idx);
//        loop_idx_load->setAlignment(8);
//        // extract the specific element from the full input array
//        std::vector<llvm::Value *> element_idxs;
//        element_idxs.push_back(loop_idx_load);
//        llvm::Value *input_set_element_gep = jit->get_builder().CreateInBoundsGEP(input_arg_load, element_idxs);
//        llvm::LoadInst *element_load = jit->get_builder().CreateLoad(input_set_element_gep);
//        element_load->setAlignment(8);
//        llvm::AllocaInst *extern_input_arg_alloc = jit->get_builder().CreateAlloca(element_load->getType());
//        extern_input_arg_alloc->setAlignment(8);
//        jit->get_builder().CreateStore(element_load, extern_input_arg_alloc)->setAlignment(8);
//        arg_types.push_back(extern_input_arg_alloc);
//    }
//    // get the outputs (if applicable)
//    if (has_output_param) {
//        std::vector<llvm::Value *> output_element_idx;
//        output_element_idx.push_back(jit->get_builder().CreateLoad(output_idx));
//        llvm::LoadInst *output_load = jit->get_builder().CreateLoad(preallocated_output_space);
//        llvm::Value *output_gep = jit->get_builder().CreateInBoundsGEP(output_load, output_element_idx);
//        llvm::AllocaInst *output_alloc;
//        if (!is_segmentation_stage) {
//            llvm::LoadInst *output_gep_load = jit->get_builder().CreateLoad(output_gep);
//            output_alloc = jit->get_builder().CreateAlloca(output_gep_load->getType());
//            jit->get_builder().CreateStore(output_gep_load, output_alloc);
//        } else { // the argument should be a ** type, not *, so pass in the gep instead of the load
//            output_alloc = jit->get_builder().CreateAlloca(output_gep->getType());
//            jit->get_builder().CreateStore(output_gep, output_alloc);
//        }
//        arg_types.push_back(output_alloc);
//    }

    return arg_types;
}

// initialize an i64
llvm::AllocaInst *init_i64(JIT *jit, int start_val, std::string name) {
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()),
                                                              nullptr, name);
    alloc->setAlignment(8);
    jit->get_builder().CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()),
                                                          start_val), alloc)->setAlignment(8);
    return alloc;
}

llvm::AllocaInst *init_i32(JIT *jit, int start_val, std::string name) {
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(llvm::Type::getInt32Ty(llvm::getGlobalContext()),
                                                              nullptr, name);
    alloc->setAlignment(8);
    jit->get_builder().CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()),
                                                          start_val), alloc)->setAlignment(8);
    return alloc;
}

void increment_i64(JIT *jit, llvm::AllocaInst *i64_val, int step_size) {
    llvm::LoadInst *load = jit->get_builder().CreateLoad(i64_val);
    load->setAlignment(8);
    llvm::Value *add = jit->get_builder().CreateAdd(load, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), step_size));
    jit->get_builder().CreateStore(add, i64_val)->setAlignment(8);
}

void increment_i32(JIT *jit, llvm::AllocaInst *i32_val, int step_size) {
    llvm::LoadInst *load = jit->get_builder().CreateLoad(i32_val);
    load->setAlignment(8);
    llvm::Value *add = jit->get_builder().CreateAdd(load, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), step_size));
    jit->get_builder().CreateStore(add, i32_val)->setAlignment(8);
}

llvm::Value *create_loop_condition_check(JIT *jit, llvm::AllocaInst *loop_idx_alloc, llvm::AllocaInst *max_loop_bound) {
    llvm::LoadInst *load_idx = jit->get_builder().CreateLoad(loop_idx_alloc);
    load_idx->setAlignment(8);
    llvm::LoadInst *load_bound = jit->get_builder().CreateLoad(max_loop_bound);
    load_bound->setAlignment(8);
    return jit->get_builder().CreateICmpSLT(load_idx, load_bound);
}

void return_data(JIT *jit, llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *output_idx) {
    llvm::LoadInst *output_idx_load = jit->get_builder().CreateLoad(output_idx);
    output_idx_load->setAlignment(8);
    std::vector<llvm::Value *> outer_struct_gep_idxs;
    outer_struct_gep_idxs.push_back(get_i32(0));
    llvm::LoadInst *outer_struct_load = jit->get_builder().CreateLoad(wrapper_output_struct_alloc);
    outer_struct_load->setAlignment(8);
    llvm::Value *outer_struct_gep = jit->get_builder().CreateInBoundsGEP(outer_struct_load, outer_struct_gep_idxs);
    llvm::LoadInst *inner_struct_load = jit->get_builder().CreateLoad(outer_struct_gep);
    inner_struct_load->setAlignment(8);
    llvm::AllocaInst *inner_struct_alloc = jit->get_builder().CreateAlloca(inner_struct_load->getType());
    jit->get_builder().CreateStore(inner_struct_load, inner_struct_alloc);
    // now we have {{X**,i32,i32}*}. get {X**,i32,i32}*
    outer_struct_gep_idxs.push_back(get_i32(0));
    llvm::Value *inner_struct_gep = jit->get_builder().CreateInBoundsGEP(inner_struct_alloc, outer_struct_gep_idxs);
    llvm::LoadInst *final_struct_ptr_load = jit->get_builder().CreateLoad(inner_struct_gep);
    final_struct_ptr_load->setAlignment(8);
    llvm::AllocaInst *final_struct_ptr_alloc = jit->get_builder().CreateAlloca(final_struct_ptr_load->getType());
    jit->get_builder().CreateStore(final_struct_ptr_load, final_struct_ptr_alloc);
    // now we finally have {X**,i32,i32}*. Get X**
    std::vector<llvm::Value *> final_struct_gep_idxs;
    final_struct_gep_idxs.push_back(get_i32(0));
    final_struct_gep_idxs.push_back(get_i32(1));
    llvm::Value *final_struct_gep = jit->get_builder().CreateInBoundsGEP(final_struct_ptr_load, final_struct_gep_idxs);
    jit->get_builder().CreateStore(jit->get_builder().CreateTruncOrBitCast(output_idx_load, llvm::Type::getInt32Ty(llvm::getGlobalContext())), final_struct_gep);
    // now reload the struct and return it
    jit->get_builder().CreateRet(jit->get_builder().CreateLoad(wrapper_output_struct_alloc));
}

// create a call to an extern function
llvm::AllocaInst *create_extern_call(JIT *jit, llvm::Function *extern_function,
                                     std::vector<llvm::AllocaInst *> extern_arg_allocs) {
    std::vector<llvm::Value *> loaded_args;
    for (std::vector<llvm::AllocaInst *>::iterator iter = extern_arg_allocs.begin();
         iter != extern_arg_allocs.end(); iter++) {
        llvm::LoadInst *arg_loaded = jit->get_builder().CreateLoad(*iter);
        arg_loaded->setAlignment(8);
        loaded_args.push_back(arg_loaded);
    }
    llvm::CallInst *extern_call_result = jit->get_builder().CreateCall(extern_function, loaded_args);
    if (extern_call_result->getType()->isVoidTy()) {
        return nullptr;
    } else {
        llvm::AllocaInst *extern_call_alloc = jit->get_builder().CreateAlloca(extern_call_result->getType());
        jit->get_builder().CreateStore(extern_call_result, extern_call_alloc);
        return extern_call_alloc;
    }
}

llvm::LoadInst *get_marray_size_field(JIT *jit, llvm::AllocaInst *marray_alloc) {
    std::vector<llvm::Value *> size_field_gep_idxs;
    size_field_gep_idxs.push_back(get_i32(0)); // step through the pointer to MArray
    size_field_gep_idxs.push_back(get_i32(1)); // the size field
    llvm::Value *size_field_gep = jit->get_builder().CreateInBoundsGEP(marray_alloc, size_field_gep_idxs);
    llvm::LoadInst *size_field_load = jit->get_builder().CreateLoad(size_field_gep);
    size_field_load->setAlignment(8);
    return size_field_load;
}

llvm::Value *get_marray_size_field_in_bytes(JIT *jit, llvm::AllocaInst *marray_alloc, llvm::Value *data_type_size) {
    llvm::LoadInst *size_field = get_marray_size_field(jit, marray_alloc);
    llvm::Value *mul = jit->get_builder().CreateMul(size_field, data_type_size);
    llvm::Value *final_size = jit->get_builder().CreateAdd(mul, get_i32(16)); // add on 16 for size of MArray
    return final_size;
}

llvm::AllocaInst *get_struct_field(JIT *jit, llvm::AllocaInst *the_struct, int field) {
    std::vector<llvm::Value *> marray_gep_idxs;
    marray_gep_idxs.push_back(get_i32(field));
    llvm::Value *marray_gep = jit->get_builder().CreateInBoundsGEP(the_struct, marray_gep_idxs);
    llvm::LoadInst *marray_load = jit->get_builder().CreateLoad(marray_gep);
    marray_load->setAlignment(8);
    llvm::AllocaInst *marray_alloc = jit->get_builder().CreateAlloca(marray_load->getType());
    marray_alloc->setAlignment(8);
    return marray_alloc;
}

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src, llvm::Value *bytes_to_copy) {
    llvm::Function *llvm_memcpy = jit->get_module()->getFunction("llvm.memcpy.p0i8.p0i8.i32");
    assert(llvm_memcpy);
    std::vector<llvm::Value *> memcpy_args;
    memcpy_args.push_back(jit->get_builder().CreateBitCast(dest, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    memcpy_args.push_back(jit->get_builder().CreateBitCast(src, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    memcpy_args.push_back(bytes_to_copy);
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), false));
    jit->get_builder().CreateCall(llvm_memcpy, memcpy_args);
}

void codegen_fprintf_int(JIT *jit, llvm::Value *the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("c_fprintf");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(as_i32(jit, the_int));
    jit->get_builder().CreateCall(c_fprintf, print_args);
}


void codegen_fprintf_int(JIT *jit, int the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("c_fprintf");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), the_int));
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
    print_args.push_back(llvm::ConstantInt::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), the_int));
    jit->get_builder().CreateCall(c_fprintf, print_args);
}

llvm::Value *codegen_c_malloc32(JIT *jit, size_t size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc_32");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), size));
    llvm::Value *malloc_field = jit->get_builder().CreateCall(c_malloc, malloc_args);
    return malloc_field;
}

llvm::Value *codegen_c_malloc64(JIT *jit, size_t size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc_64");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), size));
    llvm::Value *malloc_field = jit->get_builder().CreateCall(c_malloc, malloc_args);
    return malloc_field;
}

llvm::Value *codegen_c_malloc32(JIT *jit, llvm::Value *size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc_32");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(size);
    llvm::Value *malloc_field = jit->get_builder().CreateCall(c_malloc, malloc_args);
    return malloc_field;
}

llvm::Value *codegen_c_malloc64(JIT *jit, llvm::Value *size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc_64");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(size);
    llvm::Value *malloc_field = jit->get_builder().CreateCall(c_malloc, malloc_args);
    return malloc_field;
}

llvm::Value *codegen_llvm_ceil(JIT *jit, llvm::Value *ceil_me) {
    llvm::Function *llvm_ceil = jit->get_module()->getFunction("llvm.ceil.f32");
    assert(llvm_ceil);
    std::vector<llvm::Value *> ceil_args;
    ceil_args.push_back(ceil_me);
    return jit->get_builder().CreateCall(llvm_ceil, ceil_args);
}

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, size_t size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_malloc32(jit, size), cast_to);
}

llvm::Value *codegen_c_malloc32_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_malloc32(jit, size), cast_to);
}

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, size_t size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_malloc64(jit, size), cast_to);
}

llvm::Value *codegen_c_malloc64_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_malloc64(jit, size), cast_to);
}

// loaded_structure is the original data that was malloc'd
llvm::Value *codegen_c_realloc32(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size) {
    llvm::Function *c_realloc = jit->get_module()->getFunction("realloc_32");
    return codegen_realloc(jit, c_realloc, loaded_structure, size);
}

llvm::Value *codegen_c_realloc64(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size) {
    llvm::Function *c_realloc = jit->get_module()->getFunction("realloc_64");
    return codegen_realloc(jit, c_realloc, loaded_structure, size);
}

llvm::Value *codegen_realloc(JIT *jit, llvm::Function *c_realloc, llvm::LoadInst *loaded_structure, llvm::Value *size) {
    assert(c_realloc);
    std::vector<llvm::Value *> realloc_args;
    realloc_args.push_back(jit->get_builder().CreateBitCast(loaded_structure, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    realloc_args.push_back(size);
    llvm::Value *realloc_field = jit->get_builder().CreateCall(c_realloc, realloc_args);
    return realloc_field;
}

llvm::Value *codegen_c_realloc32_and_cast(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_realloc32(jit, loaded_structure, size), cast_to);
}

llvm::Value *codegen_c_realloc64_and_cast(JIT *jit, llvm::LoadInst *loaded_structure, llvm::Value *size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_realloc64(jit, loaded_structure, size), cast_to);
}

}