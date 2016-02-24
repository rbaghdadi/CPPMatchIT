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

// some useful things
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

int get_num_size_fields(MType *mtype) {
    switch (mtype->get_mtype_code()) {
        case mtype_file:
            return 1;
        default:
            return 0;
    }
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


// call this when your return type is a File
// these input_args are returned by load_wrapper_input_args
// first arg is the input data structure
// second is the length of all the char arrays
// third is the number of objects in the input data structure, as this will be the number of outputs objects we have

// you know what, pass in all the num elements things here instead of reading them from the input args
//void codegen_file_mempool(JIT *jit, std::vector<llvm::AllocaInst *> input_args, MFunc *mfunction) {
//    // allocate block for all the File objects, i.e. {i32,i8*}**
//    llvm::Value *file_size = get_i64(sizeof(File*));
//    llvm::LoadInst *num_file_objects = jit->get_builder().CreateLoad(input_args[2]);
//    llvm::Value *total_size = jit->get_builder().CreateMul(file_size, num_file_objects);
//    llvm::Value *file_block = codegen_c_malloc64_and_cast(jit, total_size, mfunction->get_extern_wrapper_return_type()->codegen_type());//input_args[0]->getType());
//    // allocate bloc for all the chars in all the File objects
//    llvm::Value *char_size = get_i64(sizeof(char));
//    llvm::LoadInst *num_chars = jit->get_builder().CreateLoad(input_args[1]);
//    llvm::Value *total_char_size = jit->get_builder().CreateMul(char_size, num_chars);
//
//
//}

// load up the input data
// give the inputs arguments names so we can actually use them
// the last argument is the size of the array of data being fed in. It is NOT user created
std::vector<llvm::AllocaInst *> load_wrapper_input_args(JIT *jit, llvm::Function *function) {
    llvm::Function *wrapper = function;
    unsigned ctr = 0;
    std::vector<llvm::AllocaInst *> alloc_args;
    for (llvm::Function::arg_iterator iter = wrapper->arg_begin(); iter != wrapper->arg_end(); iter++) {
//        if (ctr == mfunction->get_extern_param_types().size()) {
//            iter->setName("x_" + std::to_string(ctr++));
//            llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(iter->getType(), nullptr, "loop_bound_alloc");
//            alloc->setAlignment(8);
//            jit->get_builder().CreateStore(iter, alloc)->setAlignment(8);
//            alloc_args.push_back(alloc);
//        } else {
        iter->setName("x_" + std::to_string(ctr++));
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(iter->getType());
        alloc->setAlignment(8);
        jit->get_builder().CreateStore(iter, alloc)->setAlignment(8);
        alloc_args.push_back(alloc);
//        }
    }
    return alloc_args;
}

// load a single input argument from the wrapper inputs
std::vector<llvm::AllocaInst *> load_extern_input_arg(JIT *jit, llvm::AllocaInst *wrapper_input_arg_alloc,
                                                      llvm::AllocaInst *preallocated_output, llvm::AllocaInst *loop_idx,
                                                      bool is_segmentation_stage) {
    std::vector<llvm::AllocaInst *> arg_types;
    // load the full input array
    llvm::LoadInst *input_arg_load = jit->get_builder().CreateLoad(wrapper_input_arg_alloc);
    input_arg_load->setAlignment(8);
    llvm::LoadInst *loop_idx_load = jit->get_builder().CreateLoad(loop_idx);
    loop_idx_load->setAlignment(8);
    // extract the specific element from the full input array
    std::vector<llvm::Value *> element_idxs;
    element_idxs.push_back(loop_idx_load);
    llvm::Value *element_gep = jit->get_builder().CreateInBoundsGEP(input_arg_load, element_idxs);
    llvm::LoadInst *element_load = jit->get_builder().CreateLoad(element_gep);
    element_load->setAlignment(8);
    llvm::AllocaInst *extern_input_arg_alloc = jit->get_builder().CreateAlloca(element_load->getType());
    extern_input_arg_alloc->setAlignment(8);
    jit->get_builder().CreateStore(element_load, extern_input_arg_alloc)->setAlignment(8);
    arg_types.push_back(extern_input_arg_alloc);
    // get the outputs (if applicable)
    if (preallocated_output) {
        llvm::LoadInst *output_load = jit->get_builder().CreateLoad(preallocated_output);
        llvm::Value *output_gep = jit->get_builder().CreateInBoundsGEP(output_load, element_idxs);
        llvm::AllocaInst *output_alloc;
        if (!is_segmentation_stage) {
            llvm::LoadInst *output_gep_load = jit->get_builder().CreateLoad(output_gep);
            output_alloc = jit->get_builder().CreateAlloca(output_gep_load->getType());
            jit->get_builder().CreateStore(output_gep_load, output_alloc);
        } else { // the argument should be a ** type, not *, so pass in the gep instead of the load
            output_alloc = jit->get_builder().CreateAlloca(output_gep->getType());
            jit->get_builder().CreateStore(output_gep, output_alloc);
        }
        arg_types.push_back(output_alloc);
    }

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

void increment_i64(JIT *jit, llvm::AllocaInst *i64_val, int step_size) {
    llvm::LoadInst *load = jit->get_builder().CreateLoad(i64_val);
    load->setAlignment(8);
    llvm::Value *add = jit->get_builder().CreateAdd(load, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), step_size));
    jit->get_builder().CreateStore(add, i64_val)->setAlignment(8);
}

// Output struct has type { x**, i64}* where x is whatever struct is returned from a single application of a stage
// TODO does this need to gep through the original allocated space, or can we keep loading/allocating/storing as I do?
llvm::AllocaInst *init_wrapper_output_struct(JIT *jit, MFunc *mfunction, llvm::AllocaInst *max_loop_bound,
                                             llvm::AllocaInst *malloc_size) {
    MType *extern_wrapper_return_type = mfunction->get_extern_wrapper_return_type();
    llvm::Type *llvm_return_type = extern_wrapper_return_type->codegen_type();

    // working example: say we have our return type as WrapperOutputs of Files.
    // this looks like W = { { { { i8*, i32, i32 } * } **, i32, i32 } * } *
    // Let X = { {i8*, i32, i32} *}, i.e. the FileType class
    // this gives us { {X**, i32, i32} * } *
    // the outermost pointer is a pointer created to the WrapperOutput
    // the pointer directly to the left of that is the pointer to the MArray in WrapperOutput that holds FileType* as T

    // we have to allocate space for 3 separate components here. The first is for W. This is sizeof(W) bytes (8)
    // next, is the inner part of W, i.e.  { { { i8*, i32, i32 } * } **, i32, i32 } *. This is sizeof(MArray) bytes (16)
    // Finally, we allocate X**. This is sizeof(whatever type X* is) * num_elements

    // TODO get the number of bytes correctly, so no hardcoding (have to call the actual structure not the type itself)
    size_t wrapper_output_size = 8;
    size_t marray_output_size = 16;
    size_t inner_type_output_size = 8;

    // allocate space for W
    llvm::AllocaInst *outer_struct_alloc = jit->get_builder().CreateAlloca(llvm_return_type);
    outer_struct_alloc->setAlignment(8);
    llvm::Value *outer_struct_malloc = codegen_c_malloc64_and_cast(jit, wrapper_output_size,
                                                                   llvm_return_type);
    jit->get_builder().CreateStore(outer_struct_malloc, outer_struct_alloc)->setAlignment(8);

    // allocate space for the MArray
    std::vector<llvm::Value *> marray_gep_idxs;
    marray_gep_idxs.push_back(get_i32(0));
    llvm::Value *marray_gep_temp = jit->get_builder().CreateInBoundsGEP(outer_struct_alloc, marray_gep_idxs);
    llvm::LoadInst *marray_gep_temp_load = jit->get_builder().CreateLoad(marray_gep_temp);
    marray_gep_idxs.push_back(get_i32(0));
    llvm::Value *marray_gep_ptr = jit->get_builder().CreateInBoundsGEP(marray_gep_temp_load, marray_gep_idxs);
    llvm::LoadInst *marray_gep_ptr_load = jit->get_builder().CreateLoad(marray_gep_ptr);
    llvm::Value *marray_malloc = codegen_c_malloc64_and_cast(jit, marray_output_size, marray_gep_ptr_load->getType());
    jit->get_builder().CreateStore(marray_malloc, marray_gep_ptr);

    // allocate space for X**
    std::vector<llvm::Value *> x_gep_idxs;
    x_gep_idxs.push_back(get_i32(0));
    llvm::Value *x_gep_temp = jit->get_builder().CreateInBoundsGEP(outer_struct_alloc, x_gep_idxs);
    llvm::LoadInst *x_gep_temp_load = jit->get_builder().CreateLoad(x_gep_temp);
    x_gep_idxs.push_back(get_i32(0));
    llvm::Value *x_gep_ptr = jit->get_builder().CreateInBoundsGEP(x_gep_temp_load, x_gep_idxs);
    llvm::LoadInst *x_gep_ptr_load = jit->get_builder().CreateLoad(x_gep_ptr);
    llvm::Value *num_inner_objs = jit->get_builder().CreateLoad(max_loop_bound);
    if (mfunction->get_associated_block() == "ComparisonStage") { // TODO check the return type instead because that is safer
        num_inner_objs = jit->get_builder().CreateMul(num_inner_objs, num_inner_objs);
    }

    // now back to getting our X**
    llvm::Value *x_gep = jit->get_builder().CreateInBoundsGEP(x_gep_ptr_load, x_gep_idxs);
    llvm::LoadInst *x_gep_load = jit->get_builder().CreateLoad(x_gep);

    llvm::Value *x_malloc = codegen_c_malloc64_and_cast(jit,
                                                        jit->get_builder().CreateMul(jit->get_builder().CreateSExtOrBitCast(num_inner_objs,
                                                                                                                            llvm::Type::getInt64Ty(llvm::getGlobalContext())),
                                                                                     get_i64(inner_type_output_size)), x_gep_load->getType());
    jit->get_builder().CreateStore(x_malloc, x_gep);

    // update malloc_size_alloc to have the current number of elements we malloc'd space for
    llvm::LoadInst *load_bound = jit->get_builder().CreateLoad(max_loop_bound);
    load_bound->setAlignment(8);
    jit->get_builder().CreateStore(load_bound, malloc_size)->setAlignment(8);

    return outer_struct_alloc;
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

//llvm::Value *get_mtype_file_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc) {
//    // get the size of the filepath MArray<char>
//    llvm::AllocaInst *marray1_alloc = get_struct_field(jit, data_to_store_alloc, 0);
//    return jit->get_builder().CreateAdd(get_marray_size_field_in_bytes(jit, marray1_alloc, get_i32(1)), get_i64(8)); // add on 8 for size of File
//}
//
//llvm::Value *get_mtype_element_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *element_type) {
//    // get the size of the filepath MArray<char>
//    llvm::AllocaInst *marray_char_alloc = get_struct_field(jit, data_to_store_alloc, 0);
//    llvm::Value *marray_char_size = get_mtype_file_size(jit, marray_char_alloc);
//    // get the size of the user type MArray<T>
//    unsigned int user_type_size = element_type->get_underlying_types()[1]->get_underlying_types()[0]->
//            get_underlying_types()[0]->get_underlying_types()[0]->get_bits() / 8; // [1] => MArray<T>*, [0] => MArray<T>, [0] => T*, [0] => T
//    llvm::AllocaInst *marray_user_alloc = get_struct_field(jit, data_to_store_alloc, 1);
//    llvm::Value *marray_user_size = get_marray_size_field_in_bytes(jit, marray_user_alloc, get_i32(user_type_size));
//    llvm::Value *field_sizes = jit->get_builder().CreateAdd(marray_char_size, marray_user_size);
//    return jit->get_builder().CreateAdd(field_sizes, get_i64(16)); // add on 16 for size of Element
//}
//
//llvm::Value *get_mtype_comparison_element_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *comparison_element_type) {
//    // get the size of the two filepath MArray<char>
//    llvm::AllocaInst *marray1_char_alloc = get_struct_field(jit, data_to_store_alloc, 1); // TODO these are offset by one due to unused filepath. Fix once that is gone
//    llvm::AllocaInst *marray2_char_alloc = get_struct_field(jit, data_to_store_alloc, 2);
//    llvm::Value *marray1_char_size = get_mtype_file_size(jit, marray1_char_alloc);
//    llvm::Value *marray2_char_size = get_mtype_file_size(jit, marray2_char_alloc);
//    // get the size of the two user data MArray<T>
//    unsigned int user_type_size = comparison_element_type->get_underlying_types()[3]->get_underlying_types()[0]-> // TODO Change 3 to 2
//            get_underlying_types()[0]->get_underlying_types()[0]->get_bits() / 8; // [3] => MArray<T>*, [0] => MArray<T>, [0] => T*, [0] => T
//    llvm::AllocaInst *marray3_user_alloc = get_struct_field(jit, data_to_store_alloc, 3);
//    llvm::Value *marray3_user_size = get_marray_size_field_in_bytes(jit, marray3_user_alloc, get_i32(user_type_size));
//    llvm::Value *field_sizes = jit->get_builder().CreateAdd(marray1_char_size, jit->get_builder().CreateAdd(marray2_char_size, marray3_user_size));
//    return jit->get_builder().CreateAdd(field_sizes, get_i64(32)); // add on 20 for size of SegmentedElement
//    // TODO don't put filepath in BaseElement. Then can remove 8bytes from here for that
//}
//
//llvm::Value *get_mtype_segmented_element_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *segmented_element_type) {
//    // get the size of the filepath MArray<char>
//    llvm::AllocaInst *marray_char_alloc = get_struct_field(jit, data_to_store_alloc, 0);
//    llvm::Value *marray_char_size = get_mtype_file_size(jit, marray_char_alloc);
//    // get the size of the user type MArray<T>
//    unsigned int user_type_size = segmented_element_type->get_underlying_types()[1]->get_underlying_types()[0]->
//            get_underlying_types()[0]->get_underlying_types()[0]->get_bits() / 8; // [1] => MArray<T>*, [0] => MArray<T>, [0] => T*, [0] => T
//    llvm::AllocaInst *marray_user_alloc = get_struct_field(jit, data_to_store_alloc, 1);
//    llvm::Value *marray_user_size = get_marray_size_field_in_bytes(jit, marray_user_alloc, get_i32(user_type_size));
//    llvm::Value *field_sizes = jit->get_builder().CreateAdd(marray_char_size, marray_user_size);
//    return jit->get_builder().CreateAdd(field_sizes, get_i64(20)); // add on 20 for size of SegmentedElement
//}
//
//llvm::Value *get_mtype_segments_size(JIT *jit, llvm::AllocaInst *data_to_store_alloc, MType *segments_type, MFunc *mfunction) {
//    // get MArray<SegmentedElement<T> *> *
//    llvm::AllocaInst *marray_segmented_element_ptr_alloc = get_struct_field(jit, data_to_store_alloc, 1); // TODO this is offset by one due to unused filepath. Fix once that is gone
//    // TODO I need to get the number of SegmentedElements in here, but I also need to get the size of EACH of the SegmentedElements.
//    // get MArray<SegmentedElement<T> *>
//    std::vector<llvm::Value *> marray_segmented_element_gep_idxs;
//    marray_segmented_element_gep_idxs.push_back(get_i32(0));
//    llvm::Value *marray_segmented_element_gep = jit->get_builder().CreateInBoundsGEP(marray_segmented_element_ptr_alloc, marray_segmented_element_gep_idxs);
//    llvm::LoadInst *marray_segmented_element_load = jit->get_builder().CreateLoad(marray_segmented_element_gep);
//    // get the T* field of the above MArray (which is SegmentedElement<T> **)
//    std::vector<llvm::Value *> segmented_element_ptr_ptr_gep_idxs;
//    segmented_element_ptr_ptr_gep_idxs.push_back(get_i32(0));
//    llvm::Value *segmented_element_ptr_ptr_gep = jit->get_builder().CreateInBoundsGEP(marray_segmented_element_load, segmented_element_ptr_ptr_gep_idxs);
//    llvm::LoadInst *segmented_element_ptr_ptr_load = jit->get_builder().CreateLoad(segmented_element_ptr_ptr_gep);
//    // get the number of type of T to determine its size
//    unsigned int user_type_size = segments_type->get_underlying_types()[1]->get_underlying_types()[0]->get_underlying_types()[0]->
//            get_underlying_types()[0]->get_underlying_types()[0]->get_underlying_types()[0]->get_bits() / 8; // [1] => MArray<SegmentedElement<T>*>*,
//    // [0] => MArray<SegmentedElement<T>*>, [0] => SegmentedElement<T>*, [0] => SegmentedElement<T>, [0] => T*, [0] => T
//    // loop through all the SegmentedElement structures, getting the size of each one individually and then summing them
//    llvm::AllocaInst *cur_size = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
//    jit->get_builder().CreateStore(get_i64(0), cur_size);
//    ForLoop *segment_loop = new ForLoop(jit);
//    segment_loop->set_mfunction(mfunction);
//    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy", mfunction->get_extern_wrapper());
//    llvm::BasicBlock *body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "process_segmented_element", mfunction->get_extern_wrapper());
//    llvm::LoadInst *max_bound_load = get_marray_size_field(jit, marray_segmented_element_ptr_alloc);
//    llvm::AllocaInst *max_bound_alloc = jit->get_builder().CreateAlloca(max_bound_load->getType());
//    max_bound_alloc->setAlignment(8);
//    jit->get_builder().CreateStore(max_bound_load, max_bound_alloc)->setAlignment(8);
//    segment_loop->set_max_loop_bound(max_bound_alloc);
//    segment_loop->set_branch_to_after_counter(segment_loop->get_for_loop_condition_basic_block()->get_basic_block());
//    segment_loop->set_branch_to_true_condition(body);
//    segment_loop->set_branch_to_false_condition(dummy);
//    segment_loop->codegen();
//
//    // create the body where the size of each SegmentedElement is computed
//    jit->get_builder().SetInsertPoint(body);
//    std::vector<llvm::Value *> segmented_element_ptr_gep_idxs;
//    llvm::LoadInst *cur_segment_idx = jit->get_builder().CreateLoad(segment_loop->get_loop_counter_basic_block()->get_loop_idx_alloc());
//    segmented_element_ptr_gep_idxs.push_back(cur_segment_idx); // get the next SegmentedElement to process
//    llvm::Value *segmented_element_ptr_gep = jit->get_builder().CreateInBoundsGEP(segmented_element_ptr_ptr_load, segmented_element_ptr_gep_idxs);
//    llvm::LoadInst *segmented_element_ptr_load = jit->get_builder().CreateLoad(segmented_element_ptr_gep);
//    // Now we have SegmentedElement<T> *, get rid of the pointer
//    std::vector<llvm::Value *> segmented_element_gep_idxs;
//    segmented_element_gep_idxs.push_back(get_i32(0));
//    llvm::Value *segmented_element_gep = jit->get_builder().CreateInBoundsGEP(segmented_element_ptr_load, segmented_element_gep_idxs);
//    llvm::LoadInst *segmented_element_load = jit->get_builder().CreateLoad(segmented_element_gep);
//    llvm::AllocaInst *segmented_element_alloc = jit->get_builder().CreateAlloca(segmented_element_load->getType());
//    jit->get_builder().CreateStore(segmented_element_load, segmented_element_alloc);
//    // Now we have SegmentedElement<T>, get its size
//    llvm::Value *segmented_element_size = get_mtype_segmented_element_size(jit, segmented_element_alloc,
//                                                                           segments_type->get_underlying_types()[1]->get_underlying_types()[0]->get_underlying_types()[0]->
//                                                                                   get_underlying_types()[0]);
//    llvm::LoadInst *cur_size_load = jit->get_builder().CreateLoad(cur_size);
//    cur_size_load->setAlignment(8);
//    llvm::Value *sum = jit->get_builder().CreateAdd(segmented_element_size, cur_size_load);
//    jit->get_builder().CreateStore(sum, cur_size)->setAlignment(8);
//    jit->get_builder().CreateBr(segment_loop->get_for_loop_increment_basic_block()->get_basic_block());
//
//    jit->get_builder().SetInsertPoint(dummy);
//    llvm::LoadInst *final_size_load = jit->get_builder().CreateLoad(cur_size);
//    final_size_load->setAlignment(8);
//    return final_size_load;
//}
//
//void codegen_file_store(llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *ret_idx, MType *ret_type,
//                        llvm::AllocaInst *malloc_size_alloc, llvm::AllocaInst *extern_call_res,
//                        JIT *jit, llvm::Function *insert_into) {
//    // figure out where to store this thing
//    std::vector<llvm::Value *> output_struct_field_gep_idxs; // get out the X** from { X**, i64 }*
//    output_struct_field_gep_idxs.push_back(get_i32(0));
//    output_struct_field_gep_idxs.push_back(get_i32(0));
//    llvm::Value *output_struct_field_gep = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc,
//                                                                                output_struct_field_gep_idxs);
//    llvm::LoadInst *output_struct_field_loaded = jit->get_builder().CreateLoad(output_struct_field_gep);
//    // TODO left off here
//    // now get the correct index into X* (this is where we store the data)
////    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
////    loaded_idx->setAlignment(8);
////    std::vector<llvm::Value *> idx;
////    idx.push_back(loaded_idx);
////    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(output_struct_field_loaded, idx); // address of X[ret_idx]
////
////    // actually store the result
////    llvm::Value *ret_type_size = codegen_file_size(extern_call_res, jit);
////    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
////    // malloc space for the outer struct (8 byte pointer)
////    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
////    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
////    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
////    llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
////    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);
////
////    // increment the return idx
////    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
////    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
////    store_ret_idx->setAlignment(8);
//}
//

// ret type is the user data type that will be returned. Ex: If transform block, this is the type that comes out of the extern call
// If this is a filter block, this is the type of input data, since that is actually what is returned (not the bool that comes from the extern call)
//    void store_extern_result(JIT *jit, MType *ret_type, llvm::Value *ret_struct, llvm::Value *ret_idx,
//                             llvm::AllocaInst *extern_call_res, llvm::Function *insert_into, llvm::AllocaInst *malloc_size_alloc,
//                             llvm::BasicBlock *extern_call_store_basic_block) {
//        // So in here when we are accessing the return type with the i64 index, we need to make sure i64 is not
//        // greater than the max loop bound. If it is, then we need to make ret bigger. Actually, we kind of need a
//        // current size field (this can just go in the entry block). We commpare our index to that, reallocing
//        // if necessary. Then, we copy the realloced space over the original and then do our normal thing (hopefully
//        // this doesn't wipe out our already stored data?--it doesn't in normal C)
//        if (((MPointerType *) (ret_type))->get_underlying_type()->get_mtype_code() == mtype_file) {
//            std::cerr << "In the mtype_file store" << std::endl;
//            codegen_file_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx), ret_type,
//                               malloc_size_alloc, extern_call_res, jit, insert_into);
//        } else if (((MPointerType *) (ret_type))->get_underlying_type()->get_mtype_code() == mtype_element) {
//            std::cerr << "In the mtype_element store" << std::endl;
//            codegen_element_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx), ret_type,
//                                  malloc_size_alloc, extern_call_res, jit, insert_into);
//        } else if (((MPointerType *) (ret_type))->get_underlying_type()->get_mtype_code() == mtype_comparison_element) {
//            std::cerr << "In the mtype_comparison_element store" << std::endl;
//            codegen_comparison_element_store(llvm::cast<llvm::AllocaInst>(ret_struct),
//                                             llvm::cast<llvm::AllocaInst>(ret_idx), ret_type,
//                                             malloc_size_alloc, extern_call_res, jit, insert_into);
//        } else if (((MPointerType *) (ret_type))->get_underlying_type()->get_mtype_code() == mtype_segmented_element) {
//            std::cerr << "In the mtype_segmented_element store" << std::endl;
//            exit(5);
////            codegen_comparison_element_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx), ret_type,
////                                             malloc_size_alloc, extern_call_res, jit, insert_into);
//        } else if (((MPointerType*)(ret_type))->get_underlying_type()->get_underlying_type()->get_type_code() == mtype_segmented_element) {
//            std::cerr << "In the mtype_segments_element store" << std::endl;
//            codegen_segments_store(llvm::cast<llvm::AllocaInst>(ret_struct), llvm::cast<llvm::AllocaInst>(ret_idx),
//                                   ret_type,
//                                   malloc_size_alloc, extern_call_res, jit, insert_into);
//        } else {
//            // first, we need to load the return struct and then pull out the correct field to store the computed result
//            llvm::LoadInst *loaded_ret_struct = jit->get_builder().CreateLoad(ret_struct); // { X*, i64 }
//            std::vector<llvm::Value *> field_index;
//            field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//            field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//            llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_ret_struct, field_index); // address of X*
//            llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
//            // now get the correct index into X* (this is where we store the data)
//            llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
//            loaded_idx->setAlignment(8);
//            std::vector<llvm::Value *> idx;
//            idx.push_back(loaded_idx);
//            llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]
//            // if the user's extern function returns a pointer, we need to allocate space for that and then use a memcpy
//            if (ret_type->is_ptr_type()) {
//                mtype_code_t underlying_type = ((MPointerType *) (ret_type))->get_underlying_type()->get_mtype_code();
//                llvm::Value *ret_type_size;
//                // find out the marray/struct size (in bytes)
//                switch (underlying_type) {
//                    case mtype_element:
//                        ret_type_size = codegen_element_size(ret_type, extern_call_res, jit);
//                        break;
//                    case mtype_file:
//                        ret_type_size = codegen_file_size(extern_call_res, jit);
//                        break;
//                    case mtype_comparison_element:
//                        ret_type_size = codegen_comparison_element_size(ret_type, extern_call_res, jit);
//                        break;
//                    default:
//                        std::cerr << "Unknown mtype!" << std::endl;
//                        ret_type->dump();
//                        exit(9);
//                }
//                llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(
//                        llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8)); // 8
//                // malloc space for the outer struct (8 byte pointer)
//                llvm::Value *struct_malloc_space = codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
//                jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
//                llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
//                llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
//                codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);
//            } else {
//                // just copy it into the alloca space
//                jit->get_builder().CreateStore(extern_call_res, idx_gep);
//            }
//            // increment the return idx
//            llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
//            llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
//            store_ret_idx->setAlignment(8);
//        }
//    }
//
//
//    void return_data(JIT *jit, llvm::Value *ret, llvm::Value *ret_idx) {
//        // load the return idx
//        llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
//        loaded_ret_idx->setAlignment(8);
//
//        // load the return struct
//        llvm::LoadInst *ret_struct = jit->get_builder().CreateLoad(ret);
//
//        // store the return index in the correct field of the struct (we've already stored the actual data)
//        std::vector<llvm::Value *> field_idx;
//        field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//        field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//        llvm::Value *gep_field = jit->get_builder().CreateInBoundsGEP(ret_struct, field_idx);
//        llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(loaded_ret_idx, gep_field);
//        store_ret_idx->setAlignment(8);
//
//        // now reload the struct and return it
//        llvm::LoadInst *to_return = jit->get_builder().CreateLoad(ret);
//        jit->get_builder().CreateRet(to_return);
//    }

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
//llvm::LoadInst *get_struct_in_struct(int struct_index, llvm::AllocaInst *initial_struct, JIT *jit) {
//    llvm::LoadInst *load_initial = jit->get_builder().CreateLoad(initial_struct);
//    load_initial->setAlignment(8);
//    std::vector<llvm::Value *> marray1_gep_idx;
//    // dereference the initial pointer to get the underlying type
//    marray1_gep_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    // now pull out the actual truct
//    marray1_gep_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), struct_index));
//    llvm::Value *marray1_gep = jit->get_builder().CreateInBoundsGEP(load_initial, marray1_gep_idx);
//    llvm::LoadInst *marray1 = jit->get_builder().CreateLoad(marray1_gep);
//    marray1->setAlignment(8);
//    return marray1;
//}
//
//llvm::Value *codegen_file_size(llvm::AllocaInst *extern_call_res, JIT *jit) {
//    llvm::LoadInst *load_initial = jit->get_builder().CreateLoad(extern_call_res);
//    llvm::Value *final_size = get_marray_size_field_in_bytes(jit, load_initial, llvm::ConstantInt::get(
//            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//    return final_size;
//}
//
//llvm::Value *codegen_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit) {
//    // TODO this is super hacky, but I just want to get it working for now. I want to cry
//    // bytes per extern_input_arg_alloc
//    // in an Element, we need to access the 2nd field (the user type struct) and then the first field
//    unsigned int num_bytes = ((MPointerType*)((MStructType*)((MPointerType*)((MStructType*)(((MPointerType*)mtype)->get_underlying_type()))->get_field_types()[1])->get_underlying_type())->get_field_types()[0])->get_underlying_type()->get_bits() / 8;
//    // get the first marray (always char*)
//    llvm::LoadInst *marray1 = get_struct_in_struct(0, extern_call_res, jit);
//    llvm::Value *marray1_size = get_marray_size_field_in_bytes(jit, marray1, llvm::ConstantInt::get(
//            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//    // get the second marray
//    llvm::LoadInst *marray2 = get_struct_in_struct(1, extern_call_res, jit);
//    llvm::Value *marray2_size = get_marray_size_field_in_bytes(jit, marray2, llvm::ConstantInt::get(
//            llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
//    // add on 16 for size of Element
//    return jit->get_builder().CreateAdd(jit->get_builder().CreateAdd(marray1_size, marray2_size), llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 16));
//}
//
//llvm::Value *codegen_comparison_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit) {
//    // TODO this is super hacky, but I just want to get it working for now. I want to cry
//    // bytes per extern_input_arg_alloc
//    unsigned int num_bytes = ((MPointerType*)((MStructType*)((MPointerType*)((MStructType*)(((MPointerType*)mtype)->get_underlying_type()))->get_field_types()[2])->get_underlying_type())->get_field_types()[0])->get_underlying_type()->get_bits() / 8;
//    // get the first marray (always char*)
//    llvm::LoadInst *marray1 = get_struct_in_struct(0, extern_call_res, jit);
//    llvm::Value *marray1_size = get_marray_size_field_in_bytes(jit, marray1, llvm::ConstantInt::get(
//            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//    // get the second marray (always char*)
//    llvm::LoadInst *marray2 = get_struct_in_struct(1, extern_call_res, jit);
//    llvm::Value *marray2_size = get_marray_size_field_in_bytes(jit, marray2, llvm::ConstantInt::get(
//            llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//    // get the third marray
//    llvm::LoadInst *marray3 = get_struct_in_struct(2, extern_call_res, jit);
//    llvm::Value *marray3_size = get_marray_size_field_in_bytes(jit, marray3, llvm::ConstantInt::get(
//            llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
//    return jit->get_builder().CreateAdd(marray1_size, jit->get_builder().CreateAdd(marray2_size, marray3_size));
//}
//
//llvm::Value *codegen_segmented_element_size(MType *mtype, llvm::AllocaInst *extern_call_res, JIT *jit) {
//    // TODO this is super hacky, but I just want to get it working for now. I want to cry
//    // bytes per extern_input_arg_alloc
//    unsigned int num_bytes = ((MPointerType*)((MStructType*)((MPointerType*)((MStructType*)(((MPointerType*)mtype)->get_underlying_type()->get_underlying_type()))->get_field_types()[1])->get_underlying_type())->get_field_types()[0])->get_underlying_type()->get_bits() / 8;
//    // get the first marray (always char*)
//    llvm::LoadInst *segments_ptr = get_struct_in_struct(0, extern_call_res, jit);
//    llvm::AllocaInst *segments_ptr_alloc = jit->get_builder().CreateAlloca(segments_ptr->getType());
//    jit->get_builder().CreateStore(segments_ptr, segments_ptr_alloc);
//    llvm::LoadInst *segmented_element_ptr_ptr = get_struct_in_struct(0, segments_ptr_alloc, jit);
//    llvm::AllocaInst *segmented_element_ptr_ptr_alloc = jit->get_builder().CreateAlloca(segmented_element_ptr_ptr->getType());
//    jit->get_builder().CreateStore(segmented_element_ptr_ptr, segmented_element_ptr_ptr_alloc);
//    std::vector<llvm::Value *> ptr_ptr_gep_idxs;
//    ptr_ptr_gep_idxs.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *ptr_ptr_gep = jit->get_builder().CreateInBoundsGEP(segmented_element_ptr_ptr, ptr_ptr_gep_idxs);
//    llvm::LoadInst *segmented_element_ptr = jit->get_builder().CreateLoad(ptr_ptr_gep);
//    llvm::AllocaInst *segmented_element_ptr_alloc = jit->get_builder().CreateAlloca(segmented_element_ptr->getType());
//    jit->get_builder().CreateStore(segmented_element_ptr, segmented_element_ptr_alloc);
//    llvm::LoadInst *marray1 = get_struct_in_struct(0, segmented_element_ptr_alloc, jit);
//    marray1->setName("marray1");
//    llvm::Value *marray1_size = get_marray_size_field_in_bytes(jit, marray1, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
//
//    // get the second marray
//    llvm::LoadInst *marray2 = get_struct_in_struct(1, segmented_element_ptr_alloc, jit);
//    marray2->setName("marray2");
//    llvm::Value *marray2_size = get_marray_size_field_in_bytes(jit, marray2, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
////    llvm::LoadInst *marray2 = get_struct_in_struct(1, extern_call_res, jit);
////    llvm::Value *marray2_size = get_marray_size_field_in_bytes(jit, marray2, llvm::ConstantInt::get(
////            llvm::Type::getInt32Ty(llvm::getGlobalContext()), num_bytes));
//    // add on 16 for size of segmentedelement
//    return jit->get_builder().CreateAdd(jit->get_builder().CreateAdd(marray1_size, marray2_size), llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 32));
//}
//
//
//// create the code that defines how to store an output of type File in the overall return structure
//// assumes the initial store has happened already
//void codegen_file_store(llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *ret_idx, MType *ret_type,
//                        llvm::AllocaInst *malloc_size_alloc, llvm::AllocaInst *extern_call_res,
//                        JIT *jit, llvm::Function *insert_into) {
//    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
//    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
//    loaded_ret_idx->setAlignment(8);
//    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size_alloc);
//    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(loaded_ret_idx, loaded_malloc_size);
//    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
//    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
//    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);
//
//    // reallocate space
//    jit->get_builder().SetInsertPoint(realloc_block);
//    // TODO how much to increase space by?
//    // get the correct spot to store this in
//    std::vector<llvm::Value *> ret_data_idx;
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
//    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);
//
//    // store it
//    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single MArray pointer is 8bytes
//                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8));
//    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes, current_malloc_size_in_bytes);
//    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
//                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
//    jit->get_builder().CreateStore(reallocd, gep_ret_dat);
//    // update malloc size to have the current number of spaces (we just double it for now)
//    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, loaded_malloc_size), malloc_size_alloc)->setAlignment(8);
//    jit->get_builder().CreateBr(store_it_block);
//
//    jit->get_builder().SetInsertPoint(store_it_block);
//    // figure out where to store the result
//    std::vector<llvm::Value *> field_index;
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    std::vector<llvm::Value *> storeit_ret_data_idx;
//    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, storeit_ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);
//
//    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
//    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
//    // now get the correct index into X* (this is where we store the data)
//    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
//    loaded_idx->setAlignment(8);
//    std::vector<llvm::Value *> idx;
//    idx.push_back(loaded_idx);
//    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]
//
//    // actually store the result
//    llvm::Value *ret_type_size = codegen_file_size(extern_call_res, jit);
//    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
//    // malloc space for the outer struct (8 byte pointer)
//    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
//    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
//    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
//    llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
//    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);
//
//    // increment the return idx
//    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
//    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
//    store_ret_idx->setAlignment(8);
//}
//
//void codegen_element_store(llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *ret_idx, MType *ret_type,
//                           llvm::AllocaInst *malloc_size_alloc, llvm::AllocaInst *extern_call_res,
//                           JIT *jit, llvm::Function *insert_into) {
//    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
//    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
//    loaded_ret_idx->setAlignment(8);
//    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size_alloc);
//    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(loaded_ret_idx, loaded_malloc_size);
//    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
//    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
//    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);
//
//    // reallocate space
//    jit->get_builder().SetInsertPoint(realloc_block);
//    // TODO how much to increase space by? (this version only adds a single extern_input_arg_alloc)
//    // get the correct spot to store this malloced space in
//    std::vector<llvm::Value *> ret_data_idx;
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
//    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);
//    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single MArray pointer is 8bytes
//                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8));
//    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes, current_malloc_size_in_bytes);
//    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
//                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
//    jit->get_builder().CreateStore(reallocd, gep_ret_dat);
//    // update malloc size to have the current number of spaces (double for now)
//    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, loaded_malloc_size), malloc_size_alloc)->setAlignment(8);
//    jit->get_builder().CreateBr(store_it_block);
//
//    jit->get_builder().SetInsertPoint(store_it_block);
//    // figure out where to store the result
//    std::vector<llvm::Value *> field_index;
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    std::vector<llvm::Value *> storeit_ret_data_idx;
//    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, storeit_ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);
//
//    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
//    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
//    // now get the correct index into X* (this is where we store the data)
//    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
//    loaded_idx->setAlignment(8);
//    std::vector<llvm::Value *> idx;
//    idx.push_back(loaded_idx);
//    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]
//
//    // actually store the result
//    llvm::Value *ret_type_size = codegen_element_size(ret_type, extern_call_res, jit);
//    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
//    // malloc space for the outer struct (8 byte pointer)
//    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
//    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
//    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
//    llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
//    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);
//
//    // increment the return idx
//    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
//    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
//    store_ret_idx->setAlignment(8);
//}
//
//void codegen_comparison_element_store(llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *ret_idx, MType *ret_type,
//                                      llvm::AllocaInst *malloc_size_alloc, llvm::AllocaInst *extern_call_res,
//                                      JIT *jit, llvm::Function *insert_into) {
//    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
//    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx);
//    loaded_ret_idx->setAlignment(8);
//    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size_alloc);
//    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(loaded_ret_idx, loaded_malloc_size);
//    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
//    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
//    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);
//
//    // reallocate space
//    jit->get_builder().SetInsertPoint(realloc_block);
//    // TODO how much to increase space by? (this version only adds a single extern_input_arg_alloc)
//    // get the correct spot to store this malloced space in
//    std::vector<llvm::Value *> ret_data_idx;
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
//    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);
//    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single MArray pointer is 8bytes
//                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 8));
//    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes, current_malloc_size_in_bytes);
//    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
//                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
//    jit->get_builder().CreateStore(reallocd, gep_ret_dat);
//    // update malloc size to have the current number of spaces (double for now)
//    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, loaded_malloc_size), malloc_size_alloc)->setAlignment(8);
//    jit->get_builder().CreateBr(store_it_block);
//
//    jit->get_builder().SetInsertPoint(store_it_block);
//    // figure out where to store the result
//    std::vector<llvm::Value *> field_index;
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    std::vector<llvm::Value *> storeit_ret_data_idx;
//    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, storeit_ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);
//
//    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
//    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
//    // now get the correct index into X* (this is where we store the data)
//    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
//    loaded_idx->setAlignment(8);
//    std::vector<llvm::Value *> idx;
//    idx.push_back(loaded_idx);
//    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]
//
//    // actually store the result
//    llvm::Value *ret_type_size = codegen_comparison_element_size(ret_type, extern_call_res, jit);
//    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
//    // malloc space for the outer struct (8 byte pointer)
//    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen());
//    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
//    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
//    llvm::LoadInst *src = jit->get_builder().CreateLoad(extern_call_res);
//    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);
//
//    // increment the return idx
//    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
//    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
//    store_ret_idx->setAlignment(8);
//}
//
//// TODO this should be codegen_segments_store b/c it is specific to that
//void codegen_segments_store(llvm::AllocaInst *wrapper_output_struct_alloc, llvm::AllocaInst *ret_idx, MType *ret_type,
//                            llvm::AllocaInst *malloc_size_alloc, llvm::AllocaInst *extern_call_res,
//                            JIT *jit, llvm::Function *insert_into) {
//
//    ForLoop *segment_loop = new ForLoop(jit, insert_into); // TODO this takes an MFunc*, but I don't have one, nor do I think I need one?
//    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "dummy", insert_into);
//    llvm::BasicBlock *segment_store = llvm::BasicBlock::Create(llvm::getGlobalContext(), "segment_store", insert_into);
//
//    // first we will check to see if we have enough allocated space in the return struct. we will realloc if there isn't enough
//    llvm::LoadInst *loaded_ret_idx = jit->get_builder().CreateLoad(ret_idx, "loaded_ret_idx");
//    loaded_ret_idx->setAlignment(8);
//    llvm::LoadInst *loaded_malloc_size = jit->get_builder().CreateLoad(malloc_size_alloc, "loaded_malloc_size");
//    // for the segments block, realloc original size + the number of segments in this Segments (requires reading that field first)
//
//    llvm::LoadInst *marray1 = get_struct_in_struct(0, extern_call_res, jit);
//    llvm::Value *number_of_segments = jit->get_builder().CreateSExtOrBitCast(get_marray_size_field(jit, marray1), llvm::Type::getInt64Ty(llvm::getGlobalContext()));
//    number_of_segments->setName("number_of_segments");
//    llvm::AllocaInst *loop_bound_alloc = jit->get_builder().CreateAlloca(number_of_segments->getType());
//    jit->get_builder().CreateStore(number_of_segments, loop_bound_alloc)->setAlignment(8);
//
//    llvm::Value *is_enough = jit->get_builder().CreateICmpSLT(jit->get_builder().CreateAdd(loaded_ret_idx, number_of_segments), loaded_malloc_size);
//    llvm::BasicBlock *realloc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "realloc", insert_into);
//    llvm::BasicBlock *store_it_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store_it", insert_into);
//    jit->get_builder().CreateCondBr(is_enough, store_it_block, realloc_block);
//
//    segment_loop->set_loop_bound_alloc(loop_bound_alloc);
//    segment_loop->set_branch_to_after_counter(segment_loop->get_for_loop_condition_basic_block()->get_basic_block());
//    segment_loop->set_branch_to_true_condition(segment_store);
//    segment_loop->set_branch_to_false_condition(dummy);
//    segment_loop->codegen();
//
//    jit->get_builder().SetInsertPoint(realloc_block);
//    // get the correct spot to store this malloced space in
//    std::vector<llvm::Value *> ret_data_idx;
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_realloc_block = jit->get_builder().CreateLoad(gep_ret_dat);
//    ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    gep_ret_dat = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_realloc_block, ret_data_idx);
//    llvm::LoadInst *loaded_ret_struct_pos = jit->get_builder().CreateLoad(gep_ret_dat);
//    llvm::Value *current_malloc_size_in_bytes = jit->get_builder().CreateMul(loaded_malloc_size, // x8 because a single Segment pointer is 20bytes
//                                                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 20));
//    llvm::Value *inc_malloc_size_in_bytes = jit->get_builder().CreateAdd(current_malloc_size_in_bytes,
//                                                                         jit->get_builder().CreateMul(number_of_segments,
//                                                                                                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 20)));
//    llvm::Value *reallocd = CodegenUtils::codegen_c_realloc64_and_cast(jit, loaded_ret_struct_pos,
//                                                                       inc_malloc_size_in_bytes, loaded_ret_struct_pos->getType());
//    jit->get_builder().CreateStore(reallocd, gep_ret_dat);
//
//    // update malloc size to have the current number of spaces (double for now)
//    jit->get_builder().CreateStore(jit->get_builder().CreateAdd(loaded_malloc_size, number_of_segments), malloc_size_alloc)->setAlignment(8);
//
//    jit->get_builder().CreateBr(store_it_block);
//
//    jit->get_builder().SetInsertPoint(store_it_block);
//    // figure out where to store the result
//    std::vector<llvm::Value *> field_index;
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    field_index.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    std::vector<llvm::Value *> storeit_ret_data_idx;
//    storeit_ret_data_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
//    llvm::Value *storeit_gep_ret_dat = jit->get_builder().CreateInBoundsGEP(wrapper_output_struct_alloc, storeit_ret_data_idx);
//    llvm::LoadInst *loaded_return_struct_storeit_block = jit->get_builder().CreateLoad(storeit_gep_ret_dat);
//    llvm::Value *field_gep = jit->get_builder().CreateInBoundsGEP(loaded_return_struct_storeit_block, field_index); // address of X*
//    llvm::LoadInst *loaded_field = jit->get_builder().CreateLoad(field_gep);
//
//    // loop through all of the SegmentedElements in Segments and store each one
//    jit->get_builder().CreateBr(segment_loop->get_loop_counter_basic_block()->get_basic_block());
//
//
//    // store an individual extern_input_arg_alloc within this loop
//    jit->get_builder().SetInsertPoint(segment_store);
//    // now get the correct index into X* (this is where we store the data)
//    llvm::LoadInst *loaded_idx = jit->get_builder().CreateLoad(ret_idx);
//    loaded_idx->setAlignment(8);
//    std::vector<llvm::Value *> idx;
//    idx.push_back(loaded_idx);
//    llvm::Value *idx_gep = jit->get_builder().CreateInBoundsGEP(loaded_field, idx); // address of X[ret_idx]
//    idx_gep->setName("idx_gep");
//
//    // actually store the result
//    llvm::Value *ret_type_size = codegen_segmented_element_size(ret_type, extern_call_res, jit);
//    llvm::Value *ptr_struct_bytes = jit->get_builder().CreateAdd(ret_type_size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 8));
//    // this allocs space in our outer struct for the actual individual extern_input_arg_alloc we want to store
//    // ret_type is a pointer to a pointer, but we just want a single pointer type, so we need to get the underlying type
//    llvm::Value *struct_malloc_space = CodegenUtils::codegen_c_malloc32_and_cast(jit, ptr_struct_bytes, ret_type->codegen()->getPointerElementType());
//
//    jit->get_builder().CreateStore(struct_malloc_space, idx_gep);
//    llvm::LoadInst *dest = jit->get_builder().CreateLoad(idx_gep);
//    llvm::LoadInst *segments_source = jit->get_builder().CreateLoad(extern_call_res);
//    llvm::LoadInst *segments_ptr = get_struct_in_struct(0, extern_call_res, jit);
//    llvm::AllocaInst *segments_ptr_alloc = jit->get_builder().CreateAlloca(segments_ptr->getType());
//    jit->get_builder().CreateStore(segments_ptr, segments_ptr_alloc);
//    llvm::LoadInst *segmented_element_ptr = get_struct_in_struct(0, segments_ptr_alloc, jit);
//    std::vector<llvm::Value *> segmented_element_gep_idx;
//    llvm::LoadInst *inner_ret_idx = jit->get_builder().CreateLoad(segment_loop->get_loop_counter_basic_block()->get_loop_idx_alloc());
//    segmented_element_gep_idx.push_back(inner_ret_idx);
//    llvm::Value *segmented_element_gep = jit->get_builder().CreateInBoundsGEP(segmented_element_ptr, segmented_element_gep_idx);
//    llvm::LoadInst *src = jit->get_builder().CreateLoad(segmented_element_gep);
//    CodegenUtils::codegen_llvm_memcpy(jit, dest, src, ptr_struct_bytes);
//
//    // increment the return idx
//    llvm::Value *ret_idx_inc = jit->get_builder().CreateAdd(loaded_idx, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 1));
//    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx_inc, ret_idx);
//    store_ret_idx->setAlignment(8);
//
//    jit->get_builder().CreateBr(segment_loop->get_for_loop_increment_basic_block()->get_basic_block());
//
//    jit->get_builder().SetInsertPoint(dummy);
//
//}


/*
 * This LLVM code does the equivalent of this C code:
 *
 * T *t = (T*)malloc(sizeof(T) * num_elements * fixed_data_length);
 * Element<T> *e_ptr = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
 * Element<T> **e_ptr_ptr = (Element<T>**)malloc(sizeof(Element<T>*) * num_elements);
 *
 * for (int i = 0; i < num_elements; i++) {
 *      e_ptr[i].data = &t[i * fixed_data_length]; // kind of do a subarray slice
 *      e_ptr_ptr[i] = &e_ptr[i];
 * }
 *
 */
