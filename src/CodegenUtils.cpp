//
// Created by Jessica Ray on 1/28/16.
//

#include "./CodegenUtils.h"
#include "./MFunc.h"

namespace CodegenUtils {

// create a call to an extern function
// for the addresses, you would pass in a gep result (so needs to point to the correct element, not just all of the input data)
FuncComp create_extern_call(JIT *jit, MFunc extern_func, std::vector<llvm::Value *> extern_function_arg_addrs) {
    std::vector<llvm::Value *> loaded_args;
    for (std::vector<llvm::Value *>::iterator iter = extern_function_arg_addrs.begin();
         iter != extern_function_arg_addrs.end(); iter++) {
        llvm::LoadInst *loaded = jit->get_builder().CreateLoad(*iter);
//        loaded->setAlignment(N);
        loaded_args.push_back(loaded);
    }
    return FuncComp(jit->get_builder().CreateCall(extern_func.get_extern(), loaded_args));
}

FuncComp init_idx(JIT *jit, int start_idx, std::string name) {
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()),
                                                              nullptr, name);
    alloc->setAlignment(8);
    llvm::StoreInst *store = jit->get_builder().CreateStore(
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), start_idx),
            alloc);
    store->setAlignment(8);
    return FuncComp(alloc);
}

void increment_idx(JIT *jit, llvm::AllocaInst *loop_idx, int step_size) {
    llvm::LoadInst *load = jit->get_builder().CreateLoad(loop_idx);
    load->setAlignment(8);
    llvm::Value *add = jit->get_builder().CreateAdd(load, llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(llvm::getGlobalContext()), step_size));
    llvm::StoreInst *store = jit->get_builder().CreateStore(add, loop_idx);
    store->setAlignment(8);
}

FuncComp check_loop_idx_condition(JIT *jit, llvm::AllocaInst *loop_idx, llvm::AllocaInst *loop_bound) {
    llvm::LoadInst *load_idx = jit->get_builder().CreateLoad(loop_idx);
    load_idx->setAlignment(8);
    llvm::LoadInst *load_bound = jit->get_builder().CreateLoad(loop_bound);
    load_bound->setAlignment(8);
    llvm::Value *cmp_for_cond = jit->get_builder().CreateICmpSLT(load_idx, load_bound);
    return FuncComp(cmp_for_cond);
}

// initialize the data structure that will hold our return value
// Our return types are structs with the form: { X*, i64 }
// where X is the user type returned by the extern function and i64 is the size of the X* array
// ret type is the actual return type of the data
FuncComp init_return_data_structure(JIT *jit, MType *data_ret_type, MFunc extern_mfunc, llvm::Value *loop_bound) {
    llvm::Type *return_type = extern_mfunc.get_extern_wrapper()->getReturnType();
    // we return a pointer from the wrapper function
    llvm::Type *return_type_ptr = llvm::PointerType::get(return_type, 0);
    // allocate space for the whole return structure
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(return_type);
    // do the c-malloc
    llvm::Value *malloc_ret_struct = codegen_c_malloc_and_cast(jit, data_ret_type->get_bits() / 8 + 8, return_type);
    llvm::StoreInst *store_malloc_ret_struct = jit->get_builder().CreateStore(malloc_ret_struct, alloc);
    // now allocate space for the pointer to the returned user data (within the struct)
    llvm::LoadInst *load_bound = jit->get_builder().CreateLoad(loop_bound);
    load_bound->setAlignment(8);
    // figure out the amount of space we need for the data in the struct
    llvm::AllocaInst *alloc_num_bytes = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    alloc_num_bytes->setAlignment(8);
    unsigned int num_bytes = extern_mfunc.get_arg_types()[0]->get_bits() / 8; // int pointer, so 8bytes
    llvm::Value *llvm_num_bytes = jit->get_builder().CreateMul(load_bound, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), num_bytes));
    llvm::StoreInst *stored_bytes = jit->get_builder().CreateStore(llvm_num_bytes, alloc_num_bytes);
    stored_bytes->setAlignment(8);
    llvm::LoadInst *bytes = jit->get_builder().CreateLoad(alloc_num_bytes);
    bytes->setAlignment(8);

    llvm::Type *field_type = extern_mfunc.get_extern_wrapper()->getReturnType()->getPointerElementType()->getStructElementType(0);
    // now we call C malloc to get space for our data in the return struct (since it is always a pointer type)
    llvm::Value *malloc_data = codegen_c_malloc_and_cast(jit, bytes, field_type);
    std::vector<llvm::Value *> ret_data_idx;
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(alloc, ret_data_idx);
    llvm::LoadInst *loaded_ret_struct = jit->get_builder().CreateLoad(gep_ret_dat);
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_ret_struct, ret_data_idx);

    // now store the malloc data in the alloc space
    llvm::StoreInst *store_malloc = jit->get_builder().CreateStore(malloc_data, gep_ret_dat);

    // TODO figure out the appropriate alignment
    return FuncComp(alloc);
}

// this assumes all appropriate space has been malloc'd
// ret type is the user data type that will be returned. Ex: If transform block, this is the type that comes out of the extern call
// If this is a filter block, this is the type of input data, since that is actually what is returned (not the bool that comes from the extern call)
void store_extern_result(JIT *jit, MType *ret_type, llvm::Value *ret, llvm::Value *ret_idx,
                         llvm::Value *extern_call_res) {
    // TODO user type alignments
    // first, we need to load the return struct and then pull out the correct field to store the computed result
    llvm::LoadInst *loaded_ret_struct = jit->get_builder().CreateLoad(ret); // { X*, i64 }
    std::vector<llvm::Value *> field_index;
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_ret_struct, field_index); // address of X*
    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
    // now get the correct index into X* (this is where we store the data)
    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_idx->setAlignment(8);
    std::vector<llvm::Value *> idx;
    idx.push_back(loaded_idx);
    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]
    // if the user's extern function returns a pointer, we need to allocate space for that and then use a memcpyå
    if (ret_type->is_ptr_type()) {
        // TODO need to allocate the correct amount of space. How do I figure that out? What is the size of a user type? It could be a struct, which in that case would mean that it's 8 bytes, or if it's like a char array or something, it's the size of the array
        llvm::Value *malloc_space = codegen_c_malloc_and_cast(jit, ret_type->get_bits() * 50, ret_type->codegen());
        llvm::StoreInst *stored_malloc = jit->get_builder().CreateStore(malloc_space, idx_gep);
        llvm::LoadInst *loaded_element = jit->get_builder().CreateLoad(idx_gep);
        codegen_llvm_memcpy(jit, loaded_element, extern_call_res);
    } else {
        // just copy it into the alloca space
        llvm::StoreInst *store = jit->get_builder().CreateStore(extern_call_res, idx_gep);
    }
}

// load up the input data
// give the inputs arguments names so we can actually use them
// the last argument is the size of the array of data being fed in. It is NOT user created
FuncComp init_function_args(JIT *jit, MFunc extern_mfunc) {
    llvm::Function *wrapper = extern_mfunc.get_extern_wrapper();
    int ctr = 0;
    std::vector<llvm::Value *> args;
    for (llvm::Function::arg_iterator iter = wrapper->arg_begin(); iter != wrapper->arg_end(); iter++) {
        unsigned int alignment = 0;
        if (ctr == extern_mfunc.get_arg_types().size()) {
            alignment = 8; // this isn't one of the user types, it's the size counter going in
        } else {
            alignment = extern_mfunc.get_arg_types()[ctr]->get_bits();
        }
        iter->setName("x_" + std::to_string(ctr++));
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(iter->getType());
        alloc->setAlignment(alignment);
        llvm::StoreInst *store = jit->get_builder().CreateStore(iter, alloc);
        store->setAlignment(alignment);
        args.push_back(alloc);
    }
    return FuncComp(args);
}

void return_data(JIT *jit, llvm::Value *ret, llvm::Value *ret_idx) {
    // load the return idx
    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_ret_idx->setAlignment(8);

    // load the return struct
    llvm::LoadInst *ret_struct = jit->get_builder().CreateLoad(ret);

    // store the return index in the correct field of the struct (we've already stored the actual data)
    std::vector<llvm::Value *> field_idx;
    field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    llvm::Value *gep_field = jit->get_builder().CreateInBoundsGEP(ret_struct, field_idx);
    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(loaded_ret_idx, gep_field);
    store_ret_idx->setAlignment(8);

    // now reload the struct and return it
    llvm::LoadInst *to_return = jit->get_builder().CreateLoad(ret);
    jit->get_builder().CreateRet(to_return);
}

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src) {
    llvm::Function *llvm_memcpy = jit->get_module()->getFunction("llvm.memcpy.p0i8.p0i8.i32");
    assert(llvm_memcpy);
    std::vector<llvm::Value *> memcpy_args;
    memcpy_args.push_back(jit->get_builder().CreateBitCast(dest, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    memcpy_args.push_back(jit->get_builder().CreateBitCast(src, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    // TODO shouldn't just be 400...
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 400));
    // TODO what is the correct alignment for this?
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), false));
    jit->get_builder().CreateCall(llvm_memcpy, memcpy_args);
}


llvm::Value *codegen_c_malloc(JIT *jit, size_t size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), size));
    llvm::Value *malloc_field = jit->get_builder().CreateCall(c_malloc, malloc_args);
    return malloc_field;
}

llvm::Value *codegen_c_malloc(JIT *jit, llvm::Value *size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(size);
    llvm::Value *malloc_field = jit->get_builder().CreateCall(c_malloc, malloc_args);
    return malloc_field;
}

llvm::Value *codegen_c_malloc_and_cast(JIT *jit, size_t size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_malloc(jit, size), cast_to);
}

llvm::Value *codegen_c_malloc_and_cast(JIT *jit, llvm::Value *size, llvm::Type *cast_to) {
    return jit->get_builder().CreateBitCast(codegen_c_malloc(jit, size), cast_to);
}

}


