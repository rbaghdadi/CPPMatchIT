#ifndef MATCHIT_FIELD_H
#define MATCHIT_FIELD_H

#include "./MType.h"

class PipelineInput { };

class BaseField {
protected:
    int idx;
    void *data = nullptr;
    int cur_length;
    int max_malloc;
    int dim1;
    int dim2;
    MType *mtype;
    int element_ctr;

public:

    BaseField(int dim1, int dim2, MType *mtype) : cur_length(0), max_malloc(0), dim1(dim1), dim2(dim2), mtype(mtype),
                                                  element_ctr(0) { }

    virtual ~BaseField() { }

    void set_idx(int idx) {
        this->idx = idx;
    }

    template <typename T, int dim1, int dim2>
    T *get_array(int idx) {
        return &((T*)data)[idx];
    }

    template <typename T, int dim1, int dim2>
    T get_scalar(int idx) {
        return ((T*)data)[idx];
    }

    template <typename T, int dim1, int dim2>
    void set_array(int idx, T *values) {
        memcpy(&((T*)data)[idx], values, dim1);
    }

    template <typename T, int dim1, int dim2>
    void set_scalar(int idx, T value) {
        ((T*)data)[idx] = value;
    }

    int get_dim1() {
        return dim1;
    }

    int get_dim2() {
        return dim2;
    }

    MType *get_mtype() {
        return mtype;
    }

    int get_and_increment_ctr() {
        return element_ctr++;
    }

    // size for a single entry in a field (i.e. for a single SetElement)
    int get_fixed_size() {
        if (dim1 == 0 && dim2 == 0) {
            return 1;
        } else if (dim2 == 0) {
            return dim1;
        } else {
            return dim1 * dim2;
        }
    }

    // these aren't for the user to call

    template <typename T, int dim>
    void add_scalar(int idx, T value) {
        if (idx >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc(sizeof(T) + sizeof(T) * idx);
                max_malloc++;
            } else {
                data = realloc(data, max_malloc + sizeof(T) + sizeof(T) * idx);
                max_malloc++;
            }
        }
        ((T*)data)[idx] = value;
    }

    template <typename T, int dim>
    void add_array(int idx, const T *values) {
        if ((idx + dim) >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc(sizeof(T) * (idx + dim));
                max_malloc = idx;//+=idx;
            } else {
                data = realloc(data, sizeof(T) * (idx + dim));
                max_malloc = (idx + dim);
            }
        }
        memcpy(&((T*)data)[idx], values, dim);
    }

    template <typename T>
    void init_scalar(int id) {
        if (id >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc(sizeof(T) + sizeof(T) * id);
                max_malloc++;
            } else {
                data = realloc(data, max_malloc + sizeof(T) + sizeof(T) * id);
                max_malloc++;
            }
        }
    }

    template <typename T, int dim>
    void init_array(int id) {
        if ((idx + dim) >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc(sizeof(T) * (idx + dim));
                max_malloc = idx;//+=idx;
            } else {
                data = realloc(data, sizeof(T) * (idx + dim));
                max_malloc = (idx + dim);
            }
        }
    }

    virtual MType *to_mtype() = 0;

//    llvm::AllocaInst *allocator(JIT *jit, llvm::Type *alloca_type, llvm::Value *size_to_malloc, std::string name = "") {
//        llvm::AllocaInst *allocated_space = jit->get_builder().CreateAlloca(alloca_type, nullptr, name);
//        llvm::Value *space = CodegenUtils::codegen_c_malloc64_and_cast(jit, size_to_malloc, alloca_type);
//        jit->get_builder().CreateStore(space, allocated_space);
//        return allocated_space;
//    }
//
//    void preallocate_loop(JIT *jit, ForLoop *loop, llvm::Value *num_structs, llvm::Function *extern_wrapper_function,
//                          llvm::BasicBlock *loop_body, llvm::BasicBlock *dummy) {
//        jit->get_builder().CreateBr(loop->get_counter_bb());
//
//        // counters
//        jit->get_builder().SetInsertPoint(loop->get_counter_bb());
//        // TODO move to loop counters
//        llvm::AllocaInst *loop_bound = jit->get_builder().CreateAlloca(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
//        jit->get_builder().CreateStore(num_structs, loop_bound);
//        loop->codegen_counters(loop_bound);
//        jit->get_builder().CreateBr(loop->get_condition_bb());
//
//        // comparison
//        loop->codegen_condition();
//        jit->get_builder().CreateCondBr(loop->get_condition()->get_loop_comparison(), loop_body, dummy);
//
//        // loop increment
//        loop->codegen_loop_idx_increment();
//        jit->get_builder().CreateBr(loop->get_condition_bb());
//    }
//
//    void divide_preallocated_struct_space(JIT *jit, llvm::AllocaInst *preallocated_struct_ptr_ptr_space,
//                                          llvm::AllocaInst *preallocated_struct_ptr_space,
//                                          llvm::AllocaInst *preallocated_T_ptr_space, llvm::AllocaInst *loop_idx,
//                                          llvm::Value *T_ptr_idx, MType *mtype) {
//        llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx);
//        // e_ptr_ptr[i] = &e_ptr[i];
//        // gep e_ptr[i]
//        std::vector<llvm::Value *> preallocated_struct_ptr_gep_idx;
//        preallocated_struct_ptr_gep_idx.push_back(cur_loop_idx);
//        llvm::LoadInst *preallocated_struct_ptr_space_load = jit->get_builder().CreateLoad(preallocated_struct_ptr_space);
//        llvm::Value *preallocate_struct_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_space_load,
//                                                                                       preallocated_struct_ptr_gep_idx);
//        // gep e_ptr_ptr[i]
//        std::vector<llvm::Value *> preallocated_struct_ptr_ptr_gep_idx;
//        preallocated_struct_ptr_ptr_gep_idx.push_back(cur_loop_idx);
//        llvm::LoadInst *preallocated_struct_ptr_ptr_space_load =
//                jit->get_builder().CreateLoad(preallocated_struct_ptr_ptr_space);
//        llvm::Value *preallocate_struct_ptr_ptr_gep =
//                jit->get_builder().CreateInBoundsGEP(preallocated_struct_ptr_ptr_space_load, preallocated_struct_ptr_ptr_gep_idx);
//        jit->get_builder().CreateStore(preallocate_struct_ptr_gep, preallocate_struct_ptr_ptr_gep);
//
//        //  e_ptr[i].data = &t[T_ptr_idx];
//        std::vector<llvm::Value *> struct_ptr_data_gep_idxs;
//        struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(0));
//        // TODO this won't work for types like comparison b/c that has 2 arrays (I think?) Or can I just keep it at 1 array?
//        struct_ptr_data_gep_idxs.push_back(CodegenUtils::get_i32(mtype->get_underlying_types().size() - 1)); // get the last field which contains the data array
//        llvm::Value *struct_ptr_data_gep = jit->get_builder().CreateInBoundsGEP(preallocate_struct_ptr_gep,
//                                                                                struct_ptr_data_gep_idxs);
//        // get t[T_ptr_idx]
//        llvm::LoadInst *preallocated_T_ptr_space_load = jit->get_builder().CreateLoad(preallocated_T_ptr_space);
//        std::vector<llvm::Value *> T_ptr_gep_idx;
//        T_ptr_gep_idx.push_back(T_ptr_idx);
//        llvm::Value *T_ptr_gep = jit->get_builder().CreateInBoundsGEP(preallocated_T_ptr_space_load, T_ptr_gep_idx);
//        jit->get_builder().CreateStore(T_ptr_gep, struct_ptr_data_gep);
//    }

//    virtual void preallocate(JIT *jit, llvm::Value *num_set_elements, llvm::Function *function) = 0;

};

template <typename T, int _dim1 = 0, int _dim2 = 0>
class Field : public BaseField {
public:

    Field() : BaseField(_dim1, _dim2, create_type<T>(_dim1, _dim2)) { }

    virtual ~Field() { }

    // TODO get rid of this--can just use the MType in BaseField now
    MType *to_mtype() {
        return create_type<T>(_dim1, _dim2);
    }

//    void preallocate(JIT *jit, llvm::Value *num_set_elements, llvm::Function *function) {
//        llvm::AllocaInst *preallocated_struct_ptr_ptr_space =
//                allocator(jit, llvm::PointerType::get(llvm::PointerType::get(to_mtype()->codegen_type(), 0), 0),
//                          jit->get_builder().CreateMul(num_set_elements,
//                                                       CodegenUtils::get_i64(sizeof(T**))), "struct_ptr_ptr_pool");
//        // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//        llvm::AllocaInst *preallocated_struct_ptr_space =
//                allocator(jit, llvm::PointerType::get(to_mtype()->codegen_type(), 0),
//                          jit->get_builder().CreateMul(num_set_elements, CodegenUtils::get_i64(sizeof(T*))), "struct_ptr_pool");
//        // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
//        llvm::Value *num;
//        if (dim1 == 0 && dim2 == 0) {
//            num = num_set_elements;
//        } else if (dim2 == 0) {
//            num = jit->get_builder().CreateMul(CodegenUtils::get_i64(dim1), num_set_elements);
//        }  else {
//            num = jit->get_builder().CreateMul(num_set_elements, jit->get_builder().CreateMul(CodegenUtils::get_i64(dim1), CodegenUtils::get_i64(dim2)));
//        }
//        MType *t_type = create_type<T>();
//        llvm::AllocaInst *preallocated_T_ptr_space =
//                allocator(jit, llvm::PointerType::get(t_type->codegen_type(), 0),
//                          jit->get_builder().CreateMul(num, CodegenUtils::get_i64(sizeof(T))), "prim_ptr_pool");
//
//        // now take the memory pools and split it up evenly across num_elements
//        ForLoop loop(jit);
//        loop.init_codegen(function);
//        llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_body", function);
//        llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_dummy", function);
//        preallocate_loop(jit, &loop, num_set_elements, function, loop_body, dummy);
//        llvm::AllocaInst *loop_idx = loop.get_loop_idx();
//
//        // loop body
//        // divide up the preallocated space appropriately
//        jit->get_builder().SetInsertPoint(loop_body);
//        int fixed;
//        if (dim1 == 0 && dim2 == 0) {
//            fixed = 1;
//        } else if (dim2 == 0) {
//            fixed = dim1;
//        }  else {
//            fixed = dim1 * dim2;
//        }
//        llvm::Value *T_ptr_idx = jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop_idx), CodegenUtils::get_i64(fixed));
//        divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
//                                         preallocated_T_ptr_space, loop_idx, T_ptr_idx, to_mtype());
//        jit->get_builder().CreateBr(loop.get_increment_bb());
//
//        jit->get_builder().SetInsertPoint(dummy);
//    }

};

template <typename T>
class Field<T,0> : public BaseField {

public:

    Field() : BaseField(0, 0, create_type<T>()) { }

    virtual ~Field() { }

    MType *to_mtype() {
        return create_type<T>();
    }

//    void preallocate(JIT *jit, llvm::Value *num_set_elements, llvm::Function *function) {
//        llvm::AllocaInst *preallocated_struct_ptr_ptr_space =
//                allocator(jit, llvm::PointerType::get(llvm::PointerType::get(to_mtype()->codegen_type(), 0), 0),
//                          jit->get_builder().CreateMul(num_set_elements,
//                                                       CodegenUtils::get_i64(sizeof(T**))), "struct_ptr_ptr_pool");
//        // this is like doing Element<T> *e = (Element<T>*)malloc(sizeof(Element<T>) * num_elements);
//        llvm::AllocaInst *preallocated_struct_ptr_space =
//                allocator(jit, llvm::PointerType::get(to_mtype()->codegen_type(), 0),
//                          jit->get_builder().CreateMul(num_set_elements, CodegenUtils::get_i64(sizeof(T*))), "struct_ptr_pool");
//        // this is like doing T *t = (T*)malloc(sizeof(T) * num_prim_values)
//        llvm::Value *num = num_set_elements;
//        MType *t_type = create_type<T>();
//        llvm::AllocaInst *preallocated_T_ptr_space =
//                allocator(jit, llvm::PointerType::get(t_type->codegen_type(), 0),
//                          jit->get_builder().CreateMul(num, CodegenUtils::get_i64(sizeof(T))), "prim_ptr_pool");
//
//        // now take the memory pools and split it up evenly across num_elements
//        ForLoop loop(jit);
//        loop.init_codegen(function);
//        llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_loop_body", function);
//        llvm::BasicBlock *dummy = llvm::BasicBlock::Create(llvm::getGlobalContext(), "preallocate_dummy", function);
//        preallocate_loop(jit, &loop, num_set_elements, function, loop_body, dummy);
//        llvm::AllocaInst *loop_idx = loop.get_loop_idx();
//
//        // loop body
//        // divide up the preallocated space appropriately
//        jit->get_builder().SetInsertPoint(loop_body);
//        int fixed = 1;
//        llvm::Value *T_ptr_idx = jit->get_builder().CreateMul(jit->get_builder().CreateLoad(loop_idx), CodegenUtils::get_i64(fixed));
//        divide_preallocated_struct_space(jit, preallocated_struct_ptr_ptr_space, preallocated_struct_ptr_space,
//                                         preallocated_T_ptr_space, loop_idx, T_ptr_idx, to_mtype());
//        jit->get_builder().CreateBr(loop.get_increment_bb());
//
//        jit->get_builder().SetInsertPoint(dummy);
//    }

};

class SetElement : public PipelineInput {
    int id;
//    static int ctr;

public:

    SetElement() {  }

    template <typename T>
    T get(Field<T, 0, 0> *field) const {
        return field->template get_scalar<T,0,0>(id); // calls the BaseField func
    }

    template <typename T, int dim>
    T *get(Field<T, dim> *field) const {
        return field->template get_array<T,dim,0>(id * dim);
    }

    template <typename T, int dim>
    T get(Field<T, dim> *field, int offset) const {
        return field->template get_scalar<T,dim,0>(id * dim + offset);
    }

    template <typename T, int dim1, int dim2>
    T get(Field<T,dim1,dim2> *field, int offset1, int offset2) const {
        return field->template get_scalar<T,dim1,dim2>(dim1 * id + dim2 + dim1 * offset1 + offset2);
    }

    template <typename T>
    void set(Field<T, 0> *field, T val) {
        field->template set_scalar<T,0,0>(id, val);
    }

    template <typename T, int dim>
    void set(Field<T, dim> *field, int offset, T val) {
        field->template set_scalar<T,dim,0>(id * dim + offset, val);
    }

    template <typename T, int dim>
    void set(Field<T, dim> *field, T *vals) {
        field->template set_array<T,dim,0>(id * dim, vals);
    }

    template <typename T, int dim1, int dim2>
    void set(Field<T, dim1, dim2> *field, int offset1, int offset2, T val) {
        field->template set_scalar<T,dim1,dim2>(dim1 * id + dim2 + dim1 * offset1 + offset2, val);
    }


    int get_element_id() const {
        return id;
    }

    // These aren't for user to call
    template <typename T>
    void init(Field<T, 0> *field, T value) {
        id = field->get_and_increment_ctr();
        field->template add_scalar<T,0>(id, value); // calls the BaseField func
    }

    template <typename T, int dim>
    void init(Field<T, dim> *field, const T *values) {
        id = field->get_and_increment_ctr();
        field->template add_array<T,dim>(id * dim, values);
    }

    // These aren't for user to call
    template <typename T>
    void init_blank(Field<T, 0> *field) {
        id = field->get_and_increment_ctr();
        field->template init_scalar<T,0>(id); // calls the BaseField func
    }

    template <typename T, int dim>
    void init_blank(Field<T, dim> *field) {
        id = field->get_and_increment_ctr();
        field->template init_array<T,dim>(id * dim);
    }


};

class Relation : public PipelineInput {

    std::vector<BaseField *> fields;

public:

    void add(BaseField *field) {
        field->set_idx(fields.size());
        fields.push_back(field);
    }

    std::vector<MType *> get_mtypes() {
        std::vector<MType *> mtypes;
        for(std::vector<BaseField *>::iterator iter = fields.begin(); iter != fields.end(); iter++) {
            mtypes.push_back((*iter)->to_mtype());
        }
        return mtypes;
    }

    std::vector<BaseField *> get_fields() {
        return fields;
    }

};

template <>
struct create_type<SetElement> {
    operator MStructType*() {
        std::vector<MType *> mtype;
        mtype.push_back(MScalarType::get_int_type());
        return new MStructType(mtype_struct, mtype);
    }
};

#endif //MATCHIT_FIELD_H