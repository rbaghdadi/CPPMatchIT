//
// Created by Jessica Ray on 1/28/16.
//

#include "./Block.h"

std::string Block::get_function_name() {
    return function_name;
}

MFunc *Block::get_mfunction() {
    return mfunction;
}

void Block::set_function(MFunc *mfunction) {
    this->mfunction = mfunction;
}

// all wrapper functions return structs of the user type and an int for an index
llvm::AllocaInst *Block::codegen_init_ret_stack() {
    std::vector<llvm::Type *> ret_struct_fields;
    llvm::PointerType *llvm_ret_type; // user type
    if (mfunction->get_associated_block().compare("TransformBlock") == 0 || mfunction->get_associated_block().compare("ComparisonBlock") == 0) {
        llvm_ret_type = llvm::PointerType::get(mfunction->get_ret_type()->codegen(), 0);
    } else if (mfunction->get_associated_block().compare("FilterBlock") == 0) {
        llvm_ret_type = llvm::PointerType::get(mfunction->get_arg_types()[0]->codegen(), 0);
    }
    ret_struct_fields.push_back(llvm_ret_type);
    ret_struct_fields.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    llvm::StructType *ret_struct = llvm::StructType::get(llvm::getGlobalContext(), ret_struct_fields);
    llvm::PointerType *ptr_to_struct = llvm::PointerType::get(ret_struct, 0);
    // we first need to allocate stack space that will hold our pointer to the malloc'd return struct
    llvm::AllocaInst *stack_alloc = jit->get_builder().CreateAlloca(ptr_to_struct, nullptr, "ret_alloc");
    stack_alloc->setAlignment(mfunction->get_ret_type()->get_alignment());
    return stack_alloc;
}

llvm::Value * Block::codegen_init_ret_heap() {
    // now we call C malloc to get space for our return struct on the heap
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    llvm::Value *num_bytes = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), (mfunction->get_ret_type()->get_bits() / 8) + 8); // +8 for the i64 index
    malloc_args.push_back(num_bytes);
    llvm::CallInst *heap_alloc = jit->get_builder().CreateCall(c_malloc, malloc_args, "ret_malloc");
    // cast the malloc call to the correct llvm type
    llvm::Value *bitcast = jit->get_builder().CreateBitCast(heap_alloc, mfunction->get_extern_wrapper()->getReturnType(), "ret_malloc_cast");
    return bitcast;
}

llvm::AllocaInst *Block::codegen_init_loop_ret_idx() {
    llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()),
                                                              nullptr, "ret_idx");
    alloc->setAlignment(8);
    llvm::StoreInst *store_init_ctr = jit->get_builder().CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), 0),
                                                                     alloc);
    store_init_ctr->setAlignment(8);
    return alloc;
}

std::vector<llvm::AllocaInst *> Block::codegen_init_args() {
    std::vector<llvm::AllocaInst *> args;
    int ctr = 0;
    for (llvm::Function::arg_iterator iter = mfunction->get_extern_wrapper()->arg_begin(); iter != mfunction->get_extern_wrapper()->arg_end(); iter++) {
        // prep for using the argument
        if (ctr < mfunction->get_arg_types().size()) { // don't count the ret index argument which we added on
            llvm::Value *arg = iter;
            iter->setName("x_" + std::to_string(ctr));

            // allocate space and load the arguments
            unsigned int alignment = mfunction->get_arg_types()[ctr]->get_alignment();
            llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(
                    llvm::PointerType::get(mfunction->get_arg_types()[ctr]->codegen(), 0));
            alloc->setAlignment(alignment);
            llvm::StoreInst *store = jit->get_builder().CreateStore(arg, alloc);
            store->setAlignment(alignment);
//            llvm::LoadInst *load = jit->get_builder().CreateLoad(alloc, "user_x_" + std::to_string(ctr));
//            load->setAlignment(alignment);
            args.push_back(alloc);
            ctr++;
        }
    }
    return args;
}

void Block::codegen_loop_end_block(llvm::BasicBlock *end_block, llvm::AllocaInst *alloc_ret,
                                   llvm::AllocaInst *alloc_ret_idx) {
    jit->get_builder().SetInsertPoint(end_block);

    // load the return idx
    llvm::LoadInst *ret_idx = jit->get_builder().CreateLoad(alloc_ret_idx);
    ret_idx->setAlignment(8);

    // load the return struct
    llvm::LoadInst *ret_struct = jit->get_builder().CreateLoad(alloc_ret);

    // store the return index in the correct field of the struct
    std::vector<llvm::Value *> gep_ret_field_idx;
    gep_ret_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0));
    gep_ret_field_idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 1));
    llvm::Value *gep_ret_field = jit->get_builder().CreateInBoundsGEP(ret_struct, gep_ret_field_idx);
    llvm::StoreInst *store_ret_idx = jit->get_builder().CreateStore(ret_idx, gep_ret_field);
    store_ret_idx->setAlignment(8);

    // now reload the struct and return it
    llvm::LoadInst *final_struct = jit->get_builder().CreateLoad(alloc_ret);
    jit->get_builder().CreateRet(final_struct);
}

llvm::Value *Block::codegen_loop_body_block(llvm::AllocaInst *alloc_loop_idx, std::vector<llvm::AllocaInst *> args) {
    std::vector<llvm::Value*> extern_call_args;
    // TODO var name should be loop idx, since we are dealing with the loop index actually...
    llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(alloc_loop_idx, "cur_loop_idx");
    int iter_ctr = 0;
    // load data that is passed to the extern function
    for (llvm::Function::arg_iterator iter = mfunction->get_extern_wrapper()->arg_begin(); iter != mfunction->get_extern_wrapper()->arg_end(); iter++) {
        if (iter_ctr < mfunction->get_arg_types().size()) {
            unsigned int alignment = mfunction->get_arg_types()[iter_ctr]->get_alignment();
            std::vector<llvm::Value *> gep_idxs;
            gep_idxs.push_back(cur_loop_idx);
            llvm::LoadInst *loaded_arg = jit->get_builder().CreateLoad(args[iter_ctr]);
            llvm::Value *gep = jit->get_builder().CreateInBoundsGEP(loaded_arg, gep_idxs, "mem_addr");
            // create inputs to the function call
            llvm::LoadInst *load = jit->get_builder().CreateLoad(gep, "data");
            load->setAlignment(alignment);
            extern_call_args.push_back(load);
            iter_ctr++;
        }
    }
    llvm::Value *extern_ret = jit->get_builder().CreateCall(mfunction->get_extern(), extern_call_args, "extern_ret");
    return extern_ret;
}

llvm::AllocaInst *Block::codegen_init_loop_max_bound() {
    int last_arg_idx = mfunction->get_extern_wrapper()->getFunctionType()->getNumParams() - 1;
    llvm::Function::arg_iterator final_param = mfunction->get_extern_wrapper()->arg_begin();
    for (int i = 0; i < last_arg_idx; i++) {
        final_param++;
    }
    final_param->setName("x_" + std::to_string(last_arg_idx));
    llvm::AllocaInst *alloc_max_bound = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()), nullptr, "max_loop_bound_alloc");
    alloc_max_bound->setAlignment(8);
    llvm::StoreInst *store_max_bound = jit->get_builder().CreateStore(final_param, alloc_max_bound);//llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), load), alloc);
    store_max_bound->setAlignment(8);
    return alloc_max_bound;
}

llvm::Value *Block::codegen_init_ret_data_heap(llvm::AllocaInst *max_loop_bound) {
    llvm::LoadInst *load_max_bound = jit->get_builder().CreateLoad(max_loop_bound, "num_elements");
    load_max_bound->setAlignment(8);
    // figure out the amount of space we need for the data in the struct
    llvm::AllocaInst *alloc_num_bytes = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    alloc_num_bytes->setAlignment(8);
    unsigned int bytes_for_type = mfunction->get_arg_types()[0]->get_bits() / 8; // int pointer, so 8bytes
    llvm::Value *num_bytes = jit->get_builder().CreateMul(load_max_bound, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), bytes_for_type)); // will do constant folding
    llvm::StoreInst *stored_bytes = jit->get_builder().CreateStore(num_bytes, alloc_num_bytes);
    stored_bytes->setAlignment(8);

    // TODO call refactored malloc function
    // now we call C malloc to get space for our data in the return struct (since it is always a pointer type)
    llvm::Function *c_malloc = jit->get_module()->getFunction("malloc");
    assert(c_malloc);
    std::vector<llvm::Value *> malloc_args;
    malloc_args.push_back(num_bytes);
    llvm::CallInst *heap_alloc = jit->get_builder().CreateCall(c_malloc, malloc_args, "ret_data_malloc");
    // cast the malloc call to the correct llvm type
//    llvm::Value *bitcast = jit->get_builder().CreateBitCast(heap_alloc, llvm::PointerType::get(mfunction->get_ret_type()->codegen(), 0), "ret_data_malloc_cast");
//    llvm::Type *field_type = llvm::cast<llvm::StructType>(mfunction->get_extern_wrapper()->getReturnType())->getElementType(0);

    llvm::Type *field_type = mfunction->get_extern_wrapper()->getReturnType()->getPointerElementType()->getStructElementType(0);
    llvm::Type *underlying_type = field_type->getPointerElementType();

    // now get the underlying type that is store in the field
    llvm::Value *bitcast = jit->get_builder().CreateBitCast(heap_alloc, llvm::PointerType::get(underlying_type, 0), "ret_data_malloc_cast");

    return bitcast;
};

generic_codegen_tuple Block::codegen_init() {
    // create function prototypes
    mfunction->codegen_extern_proto();
    mfunction->codegen_extern_wrapper_proto();

    // build up the block
    llvm::BasicBlock *loop_idx_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.idx", mfunction->get_extern_wrapper());
    llvm::BasicBlock *ret_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ret", mfunction->get_extern_wrapper());
    llvm::BasicBlock *arg_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "args", mfunction->get_extern_wrapper());

    llvm::BasicBlock *for_body_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.body", mfunction->get_extern_wrapper());
    llvm::BasicBlock *for_end_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.end", mfunction->get_extern_wrapper());
    llvm::BasicBlock *for_cond_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.cond", mfunction->get_extern_wrapper());
    llvm::BasicBlock *for_inc_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.inc", mfunction->get_extern_wrapper());

    // initialize the loop counter
    jit->get_builder().SetInsertPoint(loop_idx_block);
    llvm::AllocaInst *alloc_loop_idx = LLVMIR::init_loop_idx(jit, 0);
    llvm::AllocaInst *loop_max_bound = codegen_init_loop_max_bound();
    LLVMIR::branch(jit, ret_block);

    // initialize the return value structures/fields
    jit->get_builder().SetInsertPoint(ret_block);
    llvm::AllocaInst *stack_alloc_ret_val = codegen_init_ret_stack();
    llvm::Value *heap_bitcast_ret_struct = codegen_init_ret_heap();
    llvm::AllocaInst *stack_alloc_ret_idx = codegen_init_loop_ret_idx();

    // TODO move this to a function
    // add the pointer to heap memory to our stack location
    // struct
    llvm::StoreInst *heap_to_stack = jit->get_builder().CreateStore(heap_bitcast_ret_struct, stack_alloc_ret_val);
    // field in struct
    llvm::Value *heap_bitcast_ret_data = codegen_init_ret_data_heap(loop_max_bound);
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
    LLVMIR::branch(jit, for_cond_block);

    // create the loop index increment
    jit->get_builder().SetInsertPoint(for_inc_block);
    LLVMIR::build_loop_inc_block(jit, alloc_loop_idx, 0, 1);
    LLVMIR::branch(jit, for_cond_block);

    // create the loop index condition comparison
    jit->get_builder().SetInsertPoint(for_cond_block);
    llvm::Value *condition = LLVMIR::build_loop_cond_block(jit, alloc_loop_idx, loop_max_bound);
    LLVMIR::branch_cond(jit, condition, for_body_block, for_end_block);

    // create the (initial) body of the loop
    // the rest of the body is Block specific and happens in the subclasses of Block
    jit->get_builder().SetInsertPoint(for_body_block);
    llvm::Value *call_res = codegen_loop_body_block(alloc_loop_idx, args);

    // TODO change this, it is so painful to look at
    generic_codegen_tuple codegen_ret(for_inc_block, for_end_block, alloc_loop_idx, stack_alloc_ret_idx,
                                      stack_alloc_ret_val, call_res, args[0]);

    return codegen_ret;
}

