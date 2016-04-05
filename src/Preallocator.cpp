//
// Created by Jessica Ray on 3/21/16.
//

#include "./Preallocator.h"
#include "./CodegenUtils.h"

using namespace Codegen;

llvm::AllocaInst *preallocate_field(JIT *jit, BaseField *base_field, llvm::Value *total_space_to_allocate) {
    // do the malloc
    llvm::AllocaInst *allocated_space = codegen_llvm_alloca(jit, llvm_int8Ptr, 8);
    llvm::Value *space = codegen_c_malloc32(jit, total_space_to_allocate);
    codegen_llvm_store(jit, space, allocated_space, 8);
    return allocated_space;
}

// shouldn't use this junk below anymore
//llvm::AllocaInst *do_malloc(JIT *jit, llvm::Type *alloca_type, llvm::Value *size_to_malloc, std::string name) {
//    llvm::AllocaInst *allocated_space = codegen_llvm_alloca(jit, alloca_type, 8, name);
//    llvm::Value *space = codegen_c_malloc32_and_cast(jit, size_to_malloc, alloca_type);
//    codegen_llvm_store(jit, space, allocated_space, 8);
//    return allocated_space;
//}
//
//void preallocate_loop(JIT *jit, ForLoop *loop, llvm::Value *num_structs, llvm::Function *stage_function,
//                      llvm::BasicBlock *loop_body, llvm::BasicBlock *dummy) {
//    jit->get_builder().CreateBr(loop->get_counter_bb());
//
//    // counters
//    jit->get_builder().SetInsertPoint(loop->get_counter_bb());
//    // TODO move to loop counters
//    llvm::AllocaInst *loop_bound = codegen_llvm_alloca(jit, llvm_int32, 4);
//    codegen_llvm_store(jit, num_structs, loop_bound, 4);
//    loop->codegen_counters(loop_bound);
//    jit->get_builder().CreateBr(loop->get_condition_bb());
//
//    // comparison
//    loop->codegen_condition();
//    jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(), loop_body, dummy);
//
//    // loop increment
//    loop->codegen_loop_idx_increment();
//    jit->get_builder().CreateBr(loop->get_condition_bb());
//}

// This doesn't work. Once we get to allocating multiple fields at once, will need something like this to break up into the Field data arrays
//void divide_preallocated_struct_space(JIT *jit, llvm::AllocaInst *preallocated_struct_ptr_ptr_space,
//                                      llvm::AllocaInst *preallocated_struct_ptr_space,
//                                      llvm::AllocaInst *preallocated_T_ptr_space, llvm::AllocaInst *loop_idx,
//                                      llvm::Value *T_ptr_idx, MType *data_mtype) {
//    llvm::LoadInst *cur_loop_idx = codegen_llvm_load(jit, loop_idx, 4);
//    // e_ptr_ptr[i] = &e_ptr[i];
//    // gep e_ptr[i]
//    std::vector<llvm::Value *> preallocated_struct_ptr_gep_idx;
//    preallocated_struct_ptr_gep_idx.push_back(cur_loop_idx);
//    llvm::LoadInst *preallocated_struct_ptr_space_load = codegen_llvm_load(jit, preallocated_struct_ptr_space, 8);
//    llvm::Value *preallocate_struct_ptr_gep = codegen_llvm_gep(jit, preallocated_struct_ptr_space_load,
//                                                               preallocated_struct_ptr_gep_idx);
//    // gep e_ptr_ptr[i]
//    std::vector<llvm::Value *> preallocated_struct_ptr_ptr_gep_idx;
//    preallocated_struct_ptr_ptr_gep_idx.push_back(cur_loop_idx);
//    llvm::LoadInst *preallocated_struct_ptr_ptr_space_load = codegen_llvm_load(jit, preallocated_struct_ptr_ptr_space,
//                                                                               8);
//    llvm::Value *preallocate_struct_ptr_ptr_gep = codegen_llvm_gep(jit,
//                                                                   preallocated_struct_ptr_ptr_space_load,
//                                                                   preallocated_struct_ptr_ptr_gep_idx);
//    codegen_llvm_store(jit, preallocate_struct_ptr_gep, preallocate_struct_ptr_ptr_gep, 8);
//
//    //  e_ptr[i].data = &t[T_ptr_idx];
//    std::vector<llvm::Value *> struct_ptr_data_gep_idxs;
//    struct_ptr_data_gep_idxs.push_back(as_i32(0));
//    // TODO this won't work for types like comparison b/c that has 2 arrays (I think?) Or can I just keep it at 1 array?
//    struct_ptr_data_gep_idxs.push_back(as_i32(data_mtype->get_underlying_types().size() - 1)); // get the last field which contains the data array
//    llvm::Value *struct_ptr_data_gep = codegen_llvm_gep(jit, preallocate_struct_ptr_gep, struct_ptr_data_gep_idxs);
//
//    // get t[T_ptr_idx]
//    llvm::LoadInst *preallocated_T_ptr_space_load = codegen_llvm_load(jit, preallocated_T_ptr_space, 8);
//    std::vector<llvm::Value *> T_ptr_gep_idx;
//    T_ptr_gep_idx.push_back(T_ptr_idx);
//    llvm::Value *T_ptr_gep = codegen_llvm_gep(jit, preallocated_T_ptr_space_load, T_ptr_gep_idx);
//    codegen_llvm_store(jit, T_ptr_gep, struct_ptr_data_gep, 8);
//}

// This isn't needed
//llvm::AllocaInst *preallocate(BaseField *base, JIT *jit, llvm::Value *num_set_elements, llvm::Function *function) {
//    llvm::AllocaInst *preallocated_struct_ptr_ptr_space =
//            do_malloc(jit, codegen_llvm_ptr(jit, codegen_llvm_ptr(jit, base->to_data_mtype()->codegen_type())),
//                      codegen_llvm_mul(jit, num_set_elements, as_i32(base->get_data_mtype()->get_size())), "struct_ptr_ptr_pool");
//    // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//    llvm::AllocaInst *preallocated_struct_ptr_space =
//            do_malloc(jit, codegen_llvm_ptr(jit, base->to_data_mtype()->codegen_type()),
//                      codegen_llvm_mul(jit, num_set_elements, as_i32(base->get_data_mtype()->get_size())), "struct_ptr_pool");
//    // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
//    llvm::Value *num;
//    if (base->get_dim1() == 0 && base->get_dim2() == 0) {
//        num = num_set_elements;
//    } else if (base->get_dim2() == 0) {
//        num = codegen_llvm_mul(jit, as_i32(base->get_dim1()), num_set_elements);
//    }  else {
//        num = codegen_llvm_mul(jit, num_set_elements,
//                               codegen_llvm_mul(jit, get_i32(base->get_dim1()), as_i32(base->get_dim2())));
//    }
//    assert(base->get_data_mtype()->get_underlying_types().size() == 1); // the type shouldn't be a struct
//    MType *t_type = base->get_data_mtype()->get_underlying_types()[0];//create_type<T>();
//    llvm::AllocaInst *preallocated_T_ptr_space =
//            do_malloc(jit, codegen_llvm_ptr(jit, t_type->codegen_type()),
//                      codegen_llvm_mul(jit, num, as_i32(base->get_data_mtype()->get_underlying_types()[0]->get_size())),
//                      "T_ptr_pool");
//
//    // now take the memory pools and split it up evenly across num_elements
//    ForLoop loop(jit);
//    loop.init_codegen(function);
//    llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_body", function);
//    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_dummy", function);
//    preallocate_loop(jit, &loop, num_set_elements, function, loop_body, dummy);
//    llvm::AllocaInst *loop_idx = loop.get_loop_idx();
//
//    // loop body
//    // divide up the preallocated space appropriately
//    jit->get_builder().SetInsertPoint(loop_body);
//    int fixed;
//    if (base->get_dim1() == 0 && base->get_dim2() == 0) {
//        fixed = 1;
//    } else if (base->get_dim2() == 0) {
//        fixed = base->get_dim1();
//    }  else {
//        fixed = base->get_dim1() * base->get_dim2();
//    }
//    llvm::Value *T_ptr_idx = codegen_llvm_mul(jit, codegen_llvm_load(jit, loop_idx, 8), as_i32(fixed));
//    divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
//                                     preallocated_T_ptr_space, loop_idx, T_ptr_idx, base->to_data_mtype());
//    jit->get_builder().CreateBr(loop.get_increment_bb());
//
//    jit->get_builder().SetInsertPoint(dummy);
//
//    return preallocated_struct_ptr_ptr_space;
//}
