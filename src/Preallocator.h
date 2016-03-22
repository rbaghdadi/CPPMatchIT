//
// Created by Jessica Ray on 3/21/16.
//

#ifndef CPPMATCHIT_PREALLOCATOR_H
#define CPPMATCHIT_PREALLOCATOR_H

#include "./JIT.h"
#include "./ForLoop.h"
#include "./CodegenUtils.h"

llvm::AllocaInst *allocator(JIT *jit, llvm::Type *alloca_type, llvm::Value *size_to_malloc, std::string name = "") {
    llvm::AllocaInst *allocated_space = jit->get_builder().CreateAlloca(alloca_type, nullptr, name);
    llvm::Value *space = CodegenUtils::codegen_c_malloc32_and_cast(jit, size_to_malloc, alloca_type);
    jit->get_builder().CreateStore(space, allocated_space);
    return allocated_space;
}

void preallocate_loop(JIT *jit, ForLoop *loop, llvm::Value *num_structs, llvm::Function *stage_function,
                      llvm::BasicBlock *loop_body, llvm::BasicBlock *dummy) {
    jit->get_builder().CreateBr(loop->get_counter_bb());

    // counters
    jit->get_builder().SetInsertPoint(loop->get_counter_bb());
    // TODO move to loop counters
    llvm::AllocaInst *loop_bound = jit->get_builder().CreateAlloca(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    jit->get_builder().CreateStore(num_structs, loop_bound);
    loop->codegen_counters(loop_bound);
    jit->get_builder().CreateBr(loop->get_condition_bb());

    // comparison
    loop->codegen_condition();
    jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(), loop_body, dummy);

    // loop increment
    loop->codegen_loop_idx_increment();
    jit->get_builder().CreateBr(loop->get_condition_bb());
}

void divide_preallocated_struct_space(JIT *jit, llvm::AllocaInst *preallocated_struct_ptr_ptr_space,
                                      llvm::AllocaInst *preallocated_struct_ptr_space,
                                      llvm::AllocaInst *preallocated_T_ptr_space, llvm::AllocaInst *loop_idx,
                                      llvm::Value *T_ptr_idx, MType *mtype) {
    llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
    // e_ptr_ptr[i] = &e_ptr[i];
    // gep e_ptr[i]
    std::vector<llvm::Value *> preallocated_struct_ptr_gep_idx;
    preallocated_struct_ptr_gep_idx.push_back(cur_loop_idx);
    llvm::LoadInst *preallocated_struct_ptr_space_load = jit->get_builder().CreateLoad(preallocated_struct_ptr_space);
    llvm::Value *preallocate_struct_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_space_load,
                                                                                   preallocated_struct_ptr_gep_idx);
    // gep e_ptr_ptr[i]
    std::vector<llvm::Value *> preallocated_struct_ptr_ptr_gep_idx;
    preallocated_struct_ptr_ptr_gep_idx.push_back(cur_loop_idx);
    llvm::LoadInst *preallocated_struct_ptr_ptr_space_load =
            jit->get_builder().CreateLoad(preallocated_struct_ptr_ptr_space);
    llvm::Value *preallocate_struct_ptr_ptr_gep =
            jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_ptr_space_load, preallocated_struct_ptr_ptr_gep_idx);
    jit->get_builder().CreateStore(preallocate_struct_ptr_gep, preallocate_struct_ptr_ptr_gep);

    //  e_ptr[i].data = &t[T_ptr_idx];
    std::vector<llvm::Value *> struct_ptr_data_gep_idxs;
    struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(0));
    // TODO this won't work for types like comparison b/c that has 2 arrays (I think?) Or can I just keep it at 1 array?
    struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(mtype->get_underlying_types().size() - 1)); // get the last field which contains the data array
    llvm::Value *struct_ptr_data_gep = jit->get_builder().CreateInBoundsGEP(preallocate_struct_ptr_gep,
                                                                            struct_ptr_data_gep_idxs);
    // get t[T_ptr_idx]
    llvm::LoadInst *preallocated_T_ptr_space_load = jit->get_builder().CreateLoad(preallocated_T_ptr_space);
    std::vector<llvm::Value *> T_ptr_gep_idx;
    T_ptr_gep_idx.push_back(T_ptr_idx);
    llvm::Value *T_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_T_ptr_space_load, T_ptr_gep_idx);
    jit->get_builder().CreateStore(T_ptr_gep, struct_ptr_data_gep);
}

llvm::AllocaInst *preallocate(BaseField *base, JIT *jit, llvm::Value *num_set_elements, llvm::Function *function) {
    CodegenUtils::codegen_fprintf_int(jit, CodegenUtils::get_i32(base->get_mtype()->get_size()));
    llvm::AllocaInst *preallocated_struct_ptr_ptr_space =
            allocator(jit, llvm::PointerType::get(llvm::PointerType::get(base->to_mtype()->codegen_type(), 0), 0),
                      jit->get_builder().CreateMul(num_set_elements,
                                                   CodegenUtils::get_i32(base->get_mtype()->get_size())), "struct_ptr_ptr_pool");
    // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
    llvm::AllocaInst *preallocated_struct_ptr_space =
            allocator(jit, llvm::PointerType::get(base->to_mtype()->codegen_type(), 0),
                      jit->get_builder().CreateMul(num_set_elements, CodegenUtils::get_i32(base->get_mtype()->get_size())), "struct_ptr_pool");
    // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
    llvm::Value *num;
    if (base->get_dim1() == 0 && base->get_dim2() == 0) {
        num = num_set_elements;
    } else if (base->get_dim2() == 0) {
        num = jit->get_builder().CreateMul(CodegenUtils::get_i32(base->get_dim1()), num_set_elements);
    }  else {
        num = jit->get_builder().CreateMul(num_set_elements,
                                           jit->get_builder().CreateMul(CodegenUtils::get_i32(base->get_dim1()),
                                                                        CodegenUtils::get_i32(base->get_dim2())));
    }
    assert(base->get_mtype()->get_underlying_types().size() == 1); // the type shouldn't be a struct
    MType *t_type = base->get_mtype()->get_underlying_types()[0];//create_type<T>();
    llvm::AllocaInst *preallocated_T_ptr_space =
            allocator(jit, llvm::PointerType::get(t_type->codegen_type(), 0),
                      jit->get_builder().CreateMul(num, CodegenUtils::get_i32(base->get_mtype()->get_underlying_types()[0]->get_size())), "prim_ptr_pool");

    // now take the memory pools and split it up evenly across num_elements
    ForLoop loop(jit);
    loop.init_codegen(function);
    llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_body", function);
    llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_dummy", function);
    preallocate_loop(jit, &loop, num_set_elements, function, loop_body, dummy);
    llvm::AllocaInst *loop_idx = loop.get_loop_idx();

    // loop body
    // divide up the preallocated space appropriately
    jit->get_builder().SetInsertPoint(loop_body);
    int fixed;
    if (base->get_dim1() == 0 && base->get_dim2() == 0) {
        fixed = 1;
    } else if (base->get_dim2() == 0) {
        fixed = base->get_dim1();
    }  else {
        fixed = base->get_dim1() * base->get_dim2();
    }
    llvm::Value *T_ptr_idx = jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop_idx), CodegenUtils::get_i32(fixed));
    divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
                                     preallocated_T_ptr_space, loop_idx, T_ptr_idx, base->to_mtype());
    jit->get_builder().CreateBr(loop.get_increment_bb());

    jit->get_builder().SetInsertPoint(dummy);

    return preallocated_struct_ptr_ptr_space;
}


//llvm::AllocaInst *allocator(JIT *jit, llvm::Type *alloca_type, llvm::Value *size_to_malloc, std::string name = "") {
//    llvm::AllocaInst *allocated_space = jit->get_builder().CreateAlloca(alloca_type, nullptr, name);
//    llvm::Value *space = CodegenUtils::codegen_c_malloc64_and_cast(jit, size_to_malloc, alloca_type);
//    jit->get_builder().CreateStore(space, allocated_space);
//    return allocated_space;
//}
//
//void preallocate_loop(JIT *jit, ForLoop *loop, llvm::Value *num_structs, llvm::Function *extern_wrapper_function,
//                      llvm::BasicBlock *loop_body, llvm::BasicBlock *dummy) {
//    jit->get_builder().CreateBr(loop->get_counter_bb());
//
//    // counters
//    jit->get_builder().SetInsertPoint(loop->get_counter_bb());
//    // TODO move to loop counters
//    llvm::AllocaInst *loop_bound = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
//    jit->get_builder().CreateStore(num_structs, loop_bound);
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
//
//void divide_preallocated_struct_space(JIT *jit, llvm::AllocaInst *preallocated_struct_ptr_ptr_space,
//                                      llvm::AllocaInst *preallocated_struct_ptr_space,
//                                      llvm::AllocaInst *preallocated_T_ptr_space, llvm::AllocaInst *loop_idx,
//                                      llvm::Value *T_ptr_idx, MType *mtype) {
//    llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
//    // e_ptr_ptr[i] = &e_ptr[i];
//    // gep e_ptr[i]
//    std::vector<llvm::Value *> preallocated_struct_ptr_gep_idx;
//    preallocated_struct_ptr_gep_idx.push_back(cur_loop_idx);
//    llvm::LoadInst *preallocated_struct_ptr_space_load = jit->get_builder().CreateLoad(preallocated_struct_ptr_space);
//    llvm::Value *preallocate_struct_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_space_load,
//                                                                                   preallocated_struct_ptr_gep_idx);
//    // gep e_ptr_ptr[i]
//    std::vector<llvm::Value *> preallocated_struct_ptr_ptr_gep_idx;
//    preallocated_struct_ptr_ptr_gep_idx.push_back(cur_loop_idx);
//    llvm::LoadInst *preallocated_struct_ptr_ptr_space_load =
//            jit->get_builder().CreateLoad(preallocated_struct_ptr_ptr_space);
//    llvm::Value *preallocate_struct_ptr_ptr_gep =
//            jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_ptr_space_load, preallocated_struct_ptr_ptr_gep_idx);
//    jit->get_builder().CreateStore(preallocate_struct_ptr_gep, preallocate_struct_ptr_ptr_gep);
//
//    //  e_ptr[i].data = &t[T_ptr_idx];
//    std::vector<llvm::Value *> struct_ptr_data_gep_idxs;
//    struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(0));
//    // TODO this won't work for types like comparison b/c that has 2 arrays (I think?) Or can I just keep it at 1 array?
//    struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(mtype->get_underlying_types().size() - 1)); // get the last field which contains the data array
//    llvm::Value *struct_ptr_data_gep = jit->get_builder().CreateInBoundsGEP(preallocate_struct_ptr_gep,
//                                                                            struct_ptr_data_gep_idxs);
//    // get t[T_ptr_idx]
//    llvm::LoadInst *preallocated_T_ptr_space_load = jit->get_builder().CreateLoad(preallocated_T_ptr_space);
//    std::vector<llvm::Value *> T_ptr_gep_idx;
//    T_ptr_gep_idx.push_back(T_ptr_idx);
//    llvm::Value *T_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_T_ptr_space_load, T_ptr_gep_idx);
//    jit->get_builder().CreateStore(T_ptr_gep, struct_ptr_data_gep);
//}
//
///*
// * Field<T,dim1, dim2>
// */
//
//template <typename T, int dim1 = 0, int dim2 = 0>
//void preallocate(Field<T, dim1, dim2> *field, JIT *jit, llvm::Value *num_set_elements, llvm::Function *function) {
//    llvm::AllocaInst *preallocated_struct_ptr_ptr_space =
//            allocator(jit, llvm::PointerType::get(llvm::PointerType::get(field->to_mtype()->codegen_type(), 0), 0),
//                      jit->get_builder().CreateMul(num_set_elements,
//                                                   CodegenUtils::get_i64(sizeof(T**))), "struct_ptr_ptr_pool");
//    // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//    llvm::AllocaInst *preallocated_struct_ptr_space =
//            allocator(jit, llvm::PointerType::get(field->to_mtype()->codegen_type(), 0),
//                      jit->get_builder().CreateMul(num_set_elements, CodegenUtils::get_i64(sizeof(T*))), "struct_ptr_pool");
//    // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
//    llvm::Value *num;
//    if (dim1 == 0 && dim2 == 0) {
//        num = num_set_elements;
//    } else if (dim2 == 0) {
//        num = jit->get_builder().CreateMul(CodegenUtils::get_i64(dim1), num_set_elements);
//    }  else {
//        num = jit->get_builder().CreateMul(num_set_elements, jit->get_builder().CreateMul(CodegenUtils::get_i64(dim1), CodegenUtils::get_i64(dim2)));
//    }
//    MType *t_type = create_type<T>();
//    llvm::AllocaInst *preallocated_T_ptr_space =
//            allocator(jit, llvm::PointerType::get(t_type->codegen_type(), 0),
//                      jit->get_builder().CreateMul(num, CodegenUtils::get_i64(sizeof(T))), "prim_ptr_pool");
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
//    if (dim1 == 0 && dim2 == 0) {
//        fixed = 1;
//    } else if (dim2 == 0) {
//        fixed = dim1;
//    }  else {
//        fixed = dim1 * dim2;
//    }
//    llvm::Value *T_ptr_idx = jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop_idx), CodegenUtils::get_i64(fixed));
//    divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
//                                     preallocated_T_ptr_space, loop_idx, T_ptr_idx, field->to_mtype());
//    jit->get_builder().CreateBr(loop.get_increment_bb());
//
//    jit->get_builder().SetInsertPoint(dummy);
//}
//
///*
// * Field<T,0> (scalar)
// */
//
//template <typename T>
//void preallocate(Field<T,0> *field, JIT *jit, llvm::Value *num_set_elements, llvm::Function *function) {
//    llvm::AllocaInst *preallocated_struct_ptr_ptr_space =
//            allocator(jit, llvm::PointerType::get(llvm::PointerType::get(field->to_mtype()->codegen_type(), 0), 0),
//                      jit->get_builder().CreateMul(num_set_elements,
//                                                   CodegenUtils::get_i64(sizeof(T**))), "struct_ptr_ptr_pool");
//    // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//    llvm::AllocaInst *preallocated_struct_ptr_space =
//            allocator(jit, llvm::PointerType::get(field->to_mtype()->codegen_type(), 0),
//                      jit->get_builder().CreateMul(num_set_elements, CodegenUtils::get_i64(sizeof(T*))), "struct_ptr_pool");
//    // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
//    llvm::Value *num = num_set_elements;
//    MType *t_type = create_type<T>();
//    llvm::AllocaInst *preallocated_T_ptr_space =
//            allocator(jit, llvm::PointerType::get(t_type->codegen_type(), 0),
//                      jit->get_builder().CreateMul(num, CodegenUtils::get_i64(sizeof(T))), "prim_ptr_pool");
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
//    int fixed = 1;
//    llvm::Value *T_ptr_idx = jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop_idx), CodegenUtils::get_i64(fixed));
//    divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
//                                     preallocated_T_ptr_space, loop_idx, T_ptr_idx, field->to_mtype());
//    jit->get_builder().CreateBr(loop.get_increment_bb());
//
//    jit->get_builder().SetInsertPoint(dummy);
//}

#endif //CPPMATCHIT_PREALLOCATOR_H
