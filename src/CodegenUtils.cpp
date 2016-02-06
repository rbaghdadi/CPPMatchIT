//
// Created by Jessica Ray on 1/28/16.
//

#include "./CodegenUtils.h"
#include "./CompositeTypes.h"
#include "./MFunc.h"
#include "./InstructionBlock.h"
#include "./ForLoop.h"

namespace CodegenUtils {

// create a call to an extern function
// for the addresses, you would pass in a gep result (so needs to point to the correct element, not just all of the input data)
FuncComp create_extern_call(JIT *jit, MFunc extern_func, std::vector<llvm::Value *> extern_function_arg_allocs) {
    std::vector<llvm::Value *> loaded_args;
    for (std::vector<llvm::Value *>::iterator iter = extern_function_arg_allocs.begin();
         iter != extern_function_arg_allocs.end(); iter++) {
        llvm::LoadInst *loaded = jit->get_builder().CreateLoad(*iter);
        loaded->setAlignment(8);
        loaded_args.push_back(loaded);
    }
    llvm::CallInst *call = jit->get_builder().CreateCall(extern_func.get_extern(), loaded_args);
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(call->getType());
    jit->get_builder().CreateStore(call, alloc);
    return FuncComp(alloc);
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
                                    llvm::Value *max_num_ret_elements, llvm::AllocaInst *malloc_size) {
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
    // figure out the amount of space we need for each data element in the struct
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

    // update malloc_size to have the current number of elements we malloc'd space for
    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size);
    loaded_malloc_size->setAlignment(8);
    llvm::Value *inc_malloc_size = jit->get_builder().CreateAdd(loaded_malloc_size, load_bound);
    jit->get_builder().CreateStore(inc_malloc_size, malloc_size)->setAlignment(8);

    // TODO figure out the appropriate alignment
    return FuncComp(alloc);
}

// ret type is the user data type that will be returned. Ex: If transform block, this is the type that comes out of the extern call
// If this is a filter block, this is the type of input data, since that is actually what is returned (not the bool that comes from the extern call)
void store_extern_result(JIT *jit, MType *ret_type, llvm::Value *ret_struct, llvm::Value *ret_idx,
                         llvm::AllocaInst *extern_call_res, llvm::Function *insert_into, llvm::AllocaInst *malloc_size,
                         llvm::BasicBlock *extern_call_store_basic_block) {
    // So in here when we are accessing the return type with the i64 index, we need to make sure i64 is not
    // greater than the max loop bound. If it is, then we need to make ret bigger. Actually, we kind of need a
    // current size field (this can just go in the entry block). We commpare our index to that, reallocing
    // if necessary. Then, we copy the realloced space over the original and then do our normal thing (hopefully
    // this doesn't wipe out our already stored data?--it doesn't in normal C)
    if (((MPointerType*)(ret_type))->get_underlying_type()->get_type_code() == mtype_file) {
        std::cerr << "In the mtype_file store" << std::endl;
        codegen_file_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx), ret_type,
                           malloc_size, extern_call_res, jit, insert_into);
    } else if (((MPointerType*)(ret_type))->get_underlying_type()->get_type_code() == mtype_element) {
        std::cerr << "In the mtype_element store" << std::endl;
        codegen_element_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx), ret_type,
                              malloc_size, extern_call_res, jit, insert_into);
    } else if (((MPointerType*)(ret_type))->get_underlying_type()->get_type_code() == mtype_comparison_element) {
        std::cerr << "In the mtype_comparison_element store" << std::endl;
        codegen_comparison_element_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx), ret_type,
                                         malloc_size, extern_call_res, jit, insert_into);
    } else if (((MPointerType*)(ret_type))->get_underlying_type()->get_underlying_type()->get_type_code() == mtype_segmented_element) {
        std::cerr << "In the mtype_segmented_element store" << std::endl;
        codegen_segmented_element_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx),
                                        ret_type,
                                        malloc_size, extern_call_res, jit, insert_into);
    } else if(((MPointerType*)(ret_type))->get_underlying_type()->get_type_code() == mtype_segments) {
        std::cerr << "it's a Segments" << std::endl;
        exit(1);
    } else {
        // first, we need to load the return struct and then pull out the correct field to store the computed result
        llvm::LoadInst *loaded_ret_struct = jit->get_builder().CreateLoad(ret_struct); // { X*, i64 }
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
            mtype_code_t underlying_type = ((MPointerType *) (ret_type))->get_underlying_type()->get_type_code();
            llvm::Value *ret_type_size;
            // find out the marray/struct size (in bytes)
            switch (underlying_type) {
                case mtype_element:
                    ret_type_size = codegen_element_size(ret_type, extern_call_res, jit);
                    break;
                case mtype_file:
                    ret_type_size = codegen_file_size(extern_call_res, jit);
                    break;
                case mtype_comparison_element:
                    ret_type_size = codegen_comparison_element_size(ret_type, extern_call_res, jit);
                    break;
                default:
                    std::cerr << "Unknown mtype!" << std::endl;
                    ret_type->dump();
                    exit(9);
            }
            llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8)); // 8
            // malloc space for the outer struct (8 byte pointer)
            llvm::Value *struct_malloc_space = codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
            jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
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


void codegen_fprintf_int(JIT *jit, int the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("print_int");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), the_int));
    jit->get_builder().CreateCall(c_fprintf, print_args);
}

void codegen_fprintf_float(JIT *jit, llvm::Value *the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("print_float");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(the_int);
    jit->get_builder().CreateCall(c_fprintf, print_args);
}


void codegen_fprintf_float(JIT *jit, float the_int) {
    llvm::Function *c_fprintf = jit->get_module()->getFunction("print_float");
    assert(c_fprintf);
    std::vector<llvm::Value *> print_args;
    print_args.push_back(llvm::ConstantInt::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), the_int));
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

// helper function for the size functions that follow
llvm::LoadInst *get_struct_in_struct(int struct_index, llvm::AllocaInst *initial_struct, JIT *jit) {
    llvm::LoadInst *load_initial = jit->get_builder().CreateLoad(initial_struct);
    load_initial->setAlignment(8);
    std::vector<llvm::Value *> marray1_gep_idx;
    // dereference the initial pointer to get the underlying type
    marray1_gep_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    // now pull out the actual truct
    marray1_gep_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), struct_index));
    llvm::Value *marray1_gep = jit->get_builder().CreateInBoundsGEP(load_initial, marray1_gep_idx);
    llvm::LoadInst *marray1 = jit->get_builder().CreateLoad(marray1_gep);
    marray1->setAlignment(8);
    return marray1;
}

llvm::Value *codegen_file_size(llvm::AllocaInst *extern_call_res, JIT *jit) {
    llvm::LoadInst *load_initial = jit->get_builder().CreateLoad(extern_call_res);
    llvm::Value *final_size = codegen_marray_size_in_bytes(jit, load_initial, llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    return final_size;
}

llvm::Value *codegen_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit) {
    // TODO this is super hacky, but I just want to get it working for now. I want to cry
    // bytes per element
    // in an Element, we need to access the 2nd field (the user type struct) and then the first field
    unsigned int num_bytes = ((MPointerType*)((MStructType*)((MPointerType*)((MStructType*)(((MPointerType*)mtype)->get_underlying_type()))->get_field_types()[1])->get_underlying_type())->get_field_types()[0])->get_underlying_type()->get_bits() / 8;
    // get the first marray (always char*)
    llvm::LoadInst *marray1 = get_struct_in_struct(0, extern_call_res, jit);
    llvm::Value *marray1_size = codegen_marray_size_in_bytes(jit, marray1, llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    // get the second marray
    llvm::LoadInst *marray2 = get_struct_in_struct(1, extern_call_res, jit);
    llvm::Value *marray2_size = codegen_marray_size_in_bytes(jit, marray2, llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
    // add on 16 for size of Element
    return jit->get_builder().CreateAdd(jit->get_builder().CreateAdd(marray1_size, marray2_size), llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 16));
}

llvm::Value *codegen_comparison_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit) {
    // TODO this is super hacky, but I just want to get it working for now. I want to cry
    // bytes per element
    unsigned int num_bytes = ((MPointerType*)((MStructType*)((MPointerType*)((MStructType*)(((MPointerType*)mtype)->get_underlying_type()))->get_field_types()[2])->get_underlying_type())->get_field_types()[0])->get_underlying_type()->get_bits() / 8;
    // get the first marray (always char*)
    llvm::LoadInst *marray1 = get_struct_in_struct(0, extern_call_res, jit);
    llvm::Value *marray1_size = codegen_marray_size_in_bytes(jit, marray1, llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    // get the second marray (always char*)
    llvm::LoadInst *marray2 = get_struct_in_struct(1, extern_call_res, jit);
    llvm::Value *marray2_size = codegen_marray_size_in_bytes(jit, marray2, llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    // get the third marray
    llvm::LoadInst *marray3 = get_struct_in_struct(2, extern_call_res, jit);
    llvm::Value *marray3_size = codegen_marray_size_in_bytes(jit, marray3, llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
    return jit->get_builder().CreateAdd(marray1_size, jit->get_builder().CreateAdd(marray2_size, marray3_size));
}

llvm::Value *codegen_segmented_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit) {
    // TODO this is super hacky, but I just want to get it working for now. I want to cry
    // bytes per element
    unsigned int num_bytes = ((MPointerType*)((MStructType*)((MPointerType*)((MStructType*)(((MPointerType*)mtype)->get_underlying_type()->get_underlying_type()))->get_field_types()[1])->get_underlying_type())->get_field_types()[0])->get_underlying_type()->get_bits() / 8;
    // get the first marray (always char*)
    llvm::LoadInst *segments_ptr = get_struct_in_struct(0, extern_call_res, jit);
    llvm::AllocaInst *segments_ptr_alloc = jit->get_builder().CreateAlloca(segments_ptr->getType());
    jit->get_builder().CreateStore(segments_ptr, segments_ptr_alloc);
    llvm::LoadInst *segmented_element_ptr_ptr = get_struct_in_struct(0, segments_ptr_alloc, jit);
    llvm::AllocaInst *segmented_element_ptr_ptr_alloc = jit->get_builder().CreateAlloca(segmented_element_ptr_ptr->getType());
    jit->get_builder().CreateStore(segmented_element_ptr_ptr, segmented_element_ptr_ptr_alloc);
    std::vector<llvm::Value *> ptr_ptr_gep_idxs;
    ptr_ptr_gep_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *ptr_ptr_gep = jit->get_builder().CreateInBoundsGEP(segmented_element_ptr_ptr, ptr_ptr_gep_idxs);
    llvm::LoadInst *segmented_element_ptr = jit->get_builder().CreateLoad(ptr_ptr_gep);
    llvm::AllocaInst *segmented_element_ptr_alloc = jit->get_builder().CreateAlloca(segmented_element_ptr->getType());
    jit->get_builder().CreateStore(segmented_element_ptr, segmented_element_ptr_alloc);
    llvm::LoadInst *marray1 = get_struct_in_struct(0, segmented_element_ptr_alloc, jit);
    marray1->setName("marray1");
    llvm::Value *marray1_size = codegen_marray_size_in_bytes(jit, marray1, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));

    // get the second marray
    llvm::LoadInst *marray2 = get_struct_in_struct(1, segmented_element_ptr_alloc, jit);
    marray2->setName("marray2");
    llvm::Value *marray2_size = codegen_marray_size_in_bytes(jit, marray2, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
//    llvm::LoadInst *marray2 = get_struct_in_struct(1, extern_call_res, jit);
//    llvm::Value *marray2_size = codegen_marray_size_in_bytes(jit, marray2, llvm::ConstantInt::get(
//            llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
    // add on 16 for size of segmentedelement
    return jit->get_builder().CreateAdd(jit->get_builder().CreateAdd(marray1_size, marray2_size), llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 32));
}


// create the code that defines how to store an output of type File in the overall return structure
// assumes the initial store has happened already
void codegen_file_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
                        llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
                        JIT *jit, llvm::Function *insert_into) {
    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_ret_idx->setAlignment(8);
    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size);
    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(loaded_ret_idx, loaded_malloc_size);
    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);

    // reallocate space
    jit->get_builder().SetInsertPoint(realloc_block);
    // TODO how much to increase space by?
    // get the correct spot to store this in
    std::vector<llvm::Value *> ret_data_idx;
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, ret_data_idx);
    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);

    // store it
    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single MArray pointer is 8bytes
                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8));
    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes, current_malloc_size_in_bytes);
    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
    jit->get_builder().CreateStore(reallocd, gep_ret_dat);
    // update malloc size to have the current number of spaces (we just double it for now)
    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, loaded_malloc_size), malloc_size)->setAlignment(8);
    jit->get_builder().CreateBr(store_it_block);

    jit->get_builder().SetInsertPoint(store_it_block);
    // figure out where to store the result
    std::vector<llvm::Value *> field_index;
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    std::vector<llvm::Value *> storeit_ret_data_idx;
    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, storeit_ret_data_idx);
    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);

    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
    // now get the correct index into X* (this is where we store the data)
    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_idx->setAlignment(8);
    std::vector<llvm::Value *> idx;
    idx.push_back(loaded_idx);
    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]

    // actually store the result
    llvm::Value *ret_type_size = codegen_file_size(extern_call_res, jit);
    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
    // malloc space for the outer struct (8 byte pointer)
    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
    llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);

    // increment the return idx
    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
    store_ret_idx->setAlignment(8);
}

void codegen_element_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
                           llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
                           JIT *jit, llvm::Function *insert_into) {
    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_ret_idx->setAlignment(8);
    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size);
    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(loaded_ret_idx, loaded_malloc_size);
    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);

    // reallocate space
    jit->get_builder().SetInsertPoint(realloc_block);
    // TODO how much to increase space by? (this version only adds a single element)
    // get the correct spot to store this malloced space in
    std::vector<llvm::Value *> ret_data_idx;
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, ret_data_idx);
    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);
    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single MArray pointer is 8bytes
                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8));
    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes, current_malloc_size_in_bytes);
    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
    jit->get_builder().CreateStore(reallocd, gep_ret_dat);
    // update malloc size to have the current number of spaces (double for now)
    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, loaded_malloc_size), malloc_size)->setAlignment(8);
    jit->get_builder().CreateBr(store_it_block);

    jit->get_builder().SetInsertPoint(store_it_block);
    // figure out where to store the result
    std::vector<llvm::Value *> field_index;
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    std::vector<llvm::Value *> storeit_ret_data_idx;
    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, storeit_ret_data_idx);
    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);

    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
    // now get the correct index into X* (this is where we store the data)
    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_idx->setAlignment(8);
    std::vector<llvm::Value *> idx;
    idx.push_back(loaded_idx);
    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]

    // actually store the result
    llvm::Value *ret_type_size = codegen_element_size(ret_type, extern_call_res, jit);
    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
    // malloc space for the outer struct (8 byte pointer)
    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
    llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);

    // increment the return idx
    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
    store_ret_idx->setAlignment(8);
}

void codegen_comparison_element_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
                                      llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
                                      JIT *jit, llvm::Function *insert_into) {
    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_ret_idx->setAlignment(8);
    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size);
    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(loaded_ret_idx, loaded_malloc_size);
    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);

    // reallocate space
    jit->get_builder().SetInsertPoint(realloc_block);
    // TODO how much to increase space by? (this version only adds a single element)
    // get the correct spot to store this malloced space in
    std::vector<llvm::Value *> ret_data_idx;
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, ret_data_idx);
    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);
    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single MArray pointer is 8bytes
                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8));
    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes, current_malloc_size_in_bytes);
    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
    jit->get_builder().CreateStore(reallocd, gep_ret_dat);
    // update malloc size to have the current number of spaces (double for now)
    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, loaded_malloc_size), malloc_size)->setAlignment(8);
    jit->get_builder().CreateBr(store_it_block);

    jit->get_builder().SetInsertPoint(store_it_block);
    // figure out where to store the result
    std::vector<llvm::Value *> field_index;
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    std::vector<llvm::Value *> storeit_ret_data_idx;
    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, storeit_ret_data_idx);
    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);

    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
    // now get the correct index into X* (this is where we store the data)
    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_idx->setAlignment(8);
    std::vector<llvm::Value *> idx;
    idx.push_back(loaded_idx);
    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]

    // actually store the result
    llvm::Value *ret_type_size = codegen_comparison_element_size(ret_type, extern_call_res, jit);
    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
    // malloc space for the outer struct (8 byte pointer)
    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
    llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);

    // increment the return idx
    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
    store_ret_idx->setAlignment(8);
}

// for a segment block, the actual return type will be SegmentedElement**, not Segments**
void codegen_segmented_element_store(llvm::AllocaInst *return_struct, llvm::AllocaInst *ret_idx, MType *ret_type,
                                     llvm::AllocaInst *malloc_size, llvm::AllocaInst *extern_call_res,
                                     JIT *jit, llvm::Function *insert_into) {

    ForLoop *segment_loop = new ForLoop(jit, insert_into); // TODO this takes an MFunc*, but I don't have one, nor do I think I need one?
    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy", insert_into);
    llvm::BasicBlock *segment_store = llvm::BasicBlock::Create(llvm::getGlobalContext(), "segment_store", insert_into);

    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx, "loaded_ret_idx");
    loaded_ret_idx->setAlignment(8);
    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size, "loaded_malloc_size");
    // for the segments block, realloc original size + the number of segments in this Segments (requires reading that field first)
    llvm::LoadInst *marray1 = get_struct_in_struct(0, extern_call_res, jit);
    llvm::Value *number_of_segments = jit->get_builder().CreateSExtOrBitCast(codegen_marray_size_field(jit, marray1), llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    number_of_segments->setName("number_of_segments");
    llvm::AllocaInst *max_loop_bound = jit->get_builder().CreateAlloca(number_of_segments->getType());
    jit->get_builder().CreateStore(number_of_segments, max_loop_bound)->setAlignment(8);

    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(jit->get_builder().CreateAdd(loaded_ret_idx, number_of_segments), loaded_malloc_size);
    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);

    segment_loop->set_max_loop_bound(max_loop_bound);
    segment_loop->set_branch_to_after_counter(segment_loop->get_for_loop_condition_basic_block()->get_basic_block());
    segment_loop->set_branch_to_true_condition(segment_store);
    segment_loop->set_branch_to_false_condition(dummy);
    segment_loop->codegen();

    jit->get_builder().SetInsertPoint(realloc_block);
    // get the correct spot to store this malloced space in
    std::vector<llvm::Value *> ret_data_idx;
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, ret_data_idx);
    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);
    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single Segment pointer is 20bytes
                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 20));
    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes,
                                                                         jit->get_builder().CreateMul(number_of_segments,
                                                                                                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 20)));
    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
    jit->get_builder().CreateStore(reallocd, gep_ret_dat);

    // update malloc size to have the current number of spaces (double for now)
    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, number_of_segments), malloc_size)->setAlignment(8);

    jit->get_builder().CreateBr(store_it_block);

    jit->get_builder().SetInsertPoint(store_it_block);
    // figure out where to store the result
    std::vector<llvm::Value *> field_index;
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    std::vector<llvm::Value *> storeit_ret_data_idx;
    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(return_struct, storeit_ret_data_idx);
    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);
    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);

    // loop through all of the SegmentedElements in Segments and store each one
    jit->get_builder().CreateBr(segment_loop->get_loop_counter_basic_block()->get_basic_block());


    // store an individual element within this loop
    jit->get_builder().SetInsertPoint(segment_store);
    // now get the correct index into X* (this is where we store the data)
    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
    loaded_idx->setAlignment(8);
    std::vector<llvm::Value *> idx;
    idx.push_back(loaded_idx);
    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]
    idx_gep->setName("idx_gep");

    // actually store the result
    llvm::Value *ret_type_size = codegen_segmented_element_size(ret_type, extern_call_res, jit);
    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
    // this allocs space in our outer struct for the actual individual element we want to store
    // ret_type is a pointer to a pointer, but we just want a single pointer type, so we need to get the underlying type
    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen()->getPointerElementType());

    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
    llvm::LoadInst *segments_source = jit->get_builder().CreateLoad(extern_call_res);
    llvm::LoadInst *segments_ptr = get_struct_in_struct(0, extern_call_res, jit);
    llvm::AllocaInst *segments_ptr_alloc = jit->get_builder().CreateAlloca(segments_ptr->getType());
    jit->get_builder().CreateStore(segments_ptr, segments_ptr_alloc);
    llvm::LoadInst *segmented_element_ptr = get_struct_in_struct(0, segments_ptr_alloc, jit);
    std::vector<llvm::Value *> segmented_element_gep_idx;
    llvm::LoadInst *inner_ret_idx = jit->get_builder().CreateLoad(segment_loop->get_loop_counter_basic_block()->get_loop_idx());
    segmented_element_gep_idx.push_back(inner_ret_idx);
    llvm::Value *segmented_element_gep = jit->get_builder().CreateInBoundsGEP(segmented_element_ptr, segmented_element_gep_idx);
    llvm::LoadInst *src = jit->get_builder().CreateLoad(segmented_element_gep);
    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);

    // increment the return idx
    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
    store_ret_idx->setAlignment(8);

    jit->get_builder().CreateBr(segment_loop->get_for_loop_increment_basic_block()->get_basic_block());

    jit->get_builder().SetInsertPoint(dummy);

}