//
// Created by Jessica Ray on 1/28/16.
//

#include "./CodegenUtils.h"
#include "./CompositeTypes.h"
#include "./MFunc.h"

namespace CodegenUtils {

// create a call to an extern function
// for the addresses, you would pass in a gep result (so needs to point to the correct element, not just all of the input data)
FuncComp create_extern_call(JIT *jit, MFunc extern_func, std::vector<llvm::Value *> extern_function_arg_allocs) {
    std::vector<llvm::Value *> loaded_args;
    for (std::vector<llvm::Value *>::iterator iter = extern_function_arg_allocs.begin();
         iter != extern_function_arg_allocs.end(); iter++) {
        llvm::LoadInst *loaded = jit->get_builder().CreateLoad(*iter);
//        loaded->setAlignment(N);
        loaded_args.push_back(loaded);
    }
    llvm::CallInst *call = jit->get_builder().CreateCall(extern_func.get_extern(), loaded_args);
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(call->getType());
    jit->get_builder().CreateStore(call, alloc);
    return FuncComp(alloc);//FuncComp(jit->get_builder().CreateCall(extern_func.get_extern(), loaded_args));
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

FuncComp create_loop_idx_condition(JIT *jit, llvm::AllocaInst *loop_idx, llvm::AllocaInst *loop_bound) {
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
// TODO make extern_mfunc a pointer
FuncComp init_return_data_structure(JIT *jit, MType *data_ret_type, MFunc extern_mfunc,
                                    llvm::Value *max_num_ret_elements) {
    llvm::Type *return_type = extern_mfunc.get_extern_wrapper()->getReturnType();
    // we return a pointer from the wrapper function
    llvm::PointerType::get(return_type, 0);
    // allocate space for the whole return structure
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(return_type);
    // do the c-malloc
    llvm::Value *malloc_ret_struct = codegen_c_malloc64_and_cast(jit, data_ret_type->get_bits() / 8 + 8, return_type);
    jit->get_builder().CreateStore(malloc_ret_struct, alloc);
    // now allocate space for the pointer to the returned user data (within the struct)
    llvm::LoadInst *load_bound = jit->get_builder().CreateLoad(max_num_ret_elements);
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
    llvm::Value *malloc_data = codegen_c_malloc64_and_cast(jit, bytes, field_type);
    std::vector<llvm::Value *> ret_data_idx;
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(alloc, ret_data_idx);
    llvm::LoadInst *loaded_ret_struct = jit->get_builder().CreateLoad(gep_ret_dat);
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_ret_struct, ret_data_idx);

    // now store the malloc data in the alloc space
    jit->get_builder().CreateStore(malloc_data, gep_ret_dat);

    // TODO figure out the appropriate alignment
    return FuncComp(alloc);
}

// ret type is the user data type that will be returned. Ex: If transform block, this is the type that comes out of the extern call
// If this is a filter block, this is the type of input data, since that is actually what is returned (not the bool that comes from the extern call)
void store_extern_result(JIT *jit, MType *ret_type, llvm::Value *ret, llvm::Value *ret_idx,
                         llvm::AllocaInst *extern_call_res) { //llvm::AllocaInst *(sizeify)(MType *, llvm::AllocaInst *, JIT *)) {
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

    // if the user's extern function returns a pointer, we need to allocate space for that and then use a memcpy
    if (ret_type->is_ptr_type()) {
        // what to do:
        // malloc space for the whole struct object (which we can just use 8bytes for now to make things simple)
        // store that malloc'd space
        // malloc space for the marray(s) in the struct
        // store that malloc'd space in the previous malloc'd space
        // get the underlying type of this pointer
        mtype_code_t underlying_type = ((MPointerType*)(ret_type))->get_pointer_type()->get_type_code();
        llvm::AllocaInst *alloc_marray_size;
        // find out the array size (in bytes)
        switch (underlying_type) {
            case mtype_element:
                alloc_marray_size = codegen_element_size(ret_type, extern_call_res, jit);
                break;
            case mtype_file:
                alloc_marray_size = codegen_file_size(ret_type, extern_call_res, jit);
                break;
            default:
                ret_type->dump();
                exit(9);
        }
        // TODO this is specific for the File type
        alloc_marray_size->setAlignment(8);
        llvm::LoadInst *struct_bytes = jit->get_builder().CreateLoad(alloc_marray_size);
        struct_bytes->setAlignment(8);
        // allocate space for the whole struct and store it in the ret space
        // add on 8 bytes for the pointer type
        llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(struct_bytes, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));//llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 115);//8 + 8); // 8 bytes for the struct + 8 bytes for the two i32 fields
        llvm::Value *struct_malloc_space = codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
        jit->get_builder().CreateStore(struct_malloc_space, idx_gep);

        // allocate space for the char array
//        alloc_marray_size->setAlignment(8);
//        llvm::LoadInst *num_bytes = jit->get_builder().CreateLoad(alloc_marray_size);
//        num_bytes->setAlignment(8);
//
//        llvm::Value *marray_malloc_space = codegen_c_malloc32_and_cast(jit, jit->get_builder().CreateAdd(num_bytes, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 400)), llvm::Type::getInt8PtrTy(llvm::getGlobalContext()));
//
//        // figure out where to store this char array
//        llvm::LoadInst *mallocd_struct = jit->get_builder().CreateLoad(idx_gep);
//        std::vector<llvm::Value *> marray_field_idx;
//        marray_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        marray_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        llvm::Value *marray_field_gep = jit->get_builder().CreateInBoundsGEP(mallocd_struct, marray_field_idx);
//
//        // now store it
//        jit->get_builder().CreateStore(marray_malloc_space, marray_field_gep);
//        // copy over the actual data into the malloc'd space
//        // TODO do I need to copy this over in pieces?

//        codegen_fprintf_int(jit, num_bytes);


//        llvm::Value *malloc_space = codegen_c_malloc32_and_cast(jit, num_bytes, ret_type->codegen());
//        jit->get_builder().CreateStore(malloc_space, idx_gep);
        llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
        llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
        codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);



    } else {
        // just copy it into the alloca space
        jit->get_builder().CreateStore(extern_call_res, idx_gep);
    }

    // increment the return idx
    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
    store_ret_idx->setAlignment(8);
}

// load up the input data
// give the inputs arguments names so we can actually use them
// the last argument is the size of the array of data being fed in. It is NOT user created
std::vector<llvm::AllocaInst *> init_function_args(JIT *jit, MFunc extern_mfunc) {
    llvm::Function *wrapper = extern_mfunc.get_extern_wrapper();
    size_t ctr = 0;
    std::vector<llvm::AllocaInst *> args;
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
    return args;//FuncComp(args);
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

void codegen_llvm_memcpy(JIT *jit, llvm::Value *dest, llvm::Value *src, llvm::Value *bytes_to_copy) {
    llvm::Function *llvm_memcpy = jit->get_module()->getFunction("llvm.memcpy.p0i8.p0i8.i32");
    assert(llvm_memcpy);
    std::vector<llvm::Value *> memcpy_args;
    memcpy_args.push_back(jit->get_builder().CreateBitCast(dest, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
    memcpy_args.push_back(jit->get_builder().CreateBitCast(src, llvm::Type::getInt8PtrTy(llvm::getGlobalContext())));
//     TODO shouldn't just be 400...
//    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 100));
    memcpy_args.push_back(bytes_to_copy);
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    memcpy_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt1Ty(llvm::getGlobalContext()), false));
    jit->get_builder().CreateCall(llvm_memcpy, memcpy_args);
}

void codegen_fprintf_int(JIT *jit, llvm::Value *the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("print_int");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(the_int);
    jit->get_builder().CreateCall(c_fprintf, print_args);
}

llvm::Value *codegen_c_malloc32(JIT *jit, size_t size) {
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc_32");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), size));
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

}

llvm::AllocaInst *codegen_file_size(MType *mtype, llvm::AllocaInst *alloc_ret_data, JIT *jit) {

    // dereference the pointer-to-pointer
    std::vector<llvm::Value *> initial_gep_idxs;
    initial_gep_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_initial = jit->get_builder().CreateInBoundsGEP(alloc_ret_data, initial_gep_idxs);
    llvm::LoadInst *load_initial = jit->get_builder().CreateLoad(gep_initial);

    // get the correct field out of the first struct
    std::vector<llvm::Value *> gep_struct_one_field_two_idxs;
    gep_struct_one_field_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_struct_one_field_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    llvm::Value *gep_struct_one_field_two = jit->get_builder().CreateInBoundsGEP(load_initial, gep_struct_one_field_two_idxs);
    llvm::LoadInst *load_struct_one_field_two = jit->get_builder().CreateLoad(gep_struct_one_field_two);

    // get the final size
    llvm::AllocaInst *final_size = jit->get_builder().CreateAlloca(load_struct_one_field_two->getType());
    // add on the outer struct size
    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(load_struct_one_field_two, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), sizeof(File*))), final_size);

    return final_size;
}

llvm::AllocaInst *codegen_element_size(MType *mtype, llvm::AllocaInst *alloc_ret_data, JIT *jit) {
    // get the first struct out of the main Element struct
    std::vector<llvm::Value *> gep_struct_one_idxs;
    gep_struct_one_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0));
    gep_struct_one_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_struct_one = jit->get_builder().CreateInBoundsGEP(alloc_ret_data, gep_struct_one_idxs);
    llvm::LoadInst *load_struct_one = jit->get_builder().CreateLoad(gep_struct_one);
    // get the correct field out of the first struct
    std::vector<llvm::Value *> gep_struct_one_field_two_idxs;
    gep_struct_one_field_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0));
    gep_struct_one_field_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
    llvm::Value *gep_struct_one_field_two = jit->get_builder().CreateInBoundsGEP(load_struct_one, gep_struct_one_field_two_idxs);
    llvm::LoadInst *load_struct_one_field_two = jit->get_builder().CreateLoad(gep_struct_one_field_two);
    // get the size of the first struct
    llvm::Value *struct_one_field_one_size = jit->get_builder().CreateMul(load_struct_one_field_two,
                                                                          llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), mtype->get_bits() / 8)); // array length * size of array element
    llvm::Value *const_field_one_size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8);
    llvm::Value *struct_one_size = jit->get_builder().CreateAdd(struct_one_field_one_size, jit->get_builder().CreateAdd(const_field_one_size, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), mtype->get_bits() / 8)));

    // get the second struct
    std::vector<llvm::Value *> gep_struct_two_idxs;
    gep_struct_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0));
    gep_struct_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_struct_two = jit->get_builder().CreateInBoundsGEP(alloc_ret_data, gep_struct_two_idxs);
    llvm::LoadInst *load_struct_two = jit->get_builder().CreateLoad(gep_struct_two);
    // get the correct field out of the second struct
    std::vector<llvm::Value *> gep_struct_two_field_two_idxs;
    gep_struct_two_field_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0));
    gep_struct_two_field_two_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
    llvm::Value *gep_struct_two_field_two = jit->get_builder().CreateInBoundsGEP(load_struct_two, gep_struct_two_field_two_idxs);
    llvm::LoadInst *load_struct_two_field_two = jit->get_builder().CreateLoad(gep_struct_two_field_two);
    // get the size of the second struct
    llvm::Value *struct_two_field_two_size = jit->get_builder().CreateMul(load_struct_two_field_two,
                                                                          llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), mtype->get_bits() / 8)); // array length * size of array element
    llvm::Value *const_field_two_size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8);
    llvm::Value *struct_two_size = jit->get_builder().CreateAdd(struct_two_field_two_size, jit->get_builder().CreateAdd(const_field_two_size, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), mtype->get_bits() / 8)));

    // get the final size
    llvm::Value *add = jit->get_builder().CreateAdd(struct_one_size, struct_two_size);
    llvm::AllocaInst *final_add = jit->get_builder().CreateAlloca(add->getType());
    jit->get_builder().CreateStore(final_add, add);
    return final_add;
}
