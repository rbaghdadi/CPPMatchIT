#ifndef MATCHIT_FIELD_H
#define MATCHIT_FIELD_H

#include "./MType.h"
#include "./Utils.h"

// DON'T ADD VIRTUAL METHODS TO THIS OTHERWISE IT WILL BREAK!!!!
class BaseField {
protected:
    int idx;
    int cur_length;
    int max_malloc;
    int dim1;
    int dim2;
    int element_ctr;
    MType *data_mtype; // type of the data stored in this BaseField
    void *data = nullptr;

public:

    BaseField(int dim1, int dim2, MType *data_mtype) : idx(0), cur_length(0), max_malloc(0), dim1(dim1), dim2(dim2),
                                    element_ctr(0), data_mtype(data_mtype) { }

    void set_idx(int idx);

    int get_idx();

    int get_dim1();

    int get_dim2();

    int get_dummy_field();

    MType *get_data_mtype();

    int get_and_increment_ctr();

    int get_cur_length();

    bool isNull() {
        return data == nullptr;
    }

    // size for a single entry in a field (i.e. for a single SetElement)
    int get_fixed_size();

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

    // these aren't for the user to call. Only used when initializing outside of llvm

    template <typename T, int dim>
    void add_scalar(int idx, T value) {
        if (idx >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc_32(sizeof(T) + sizeof(T) * idx);
                max_malloc++;
            } else {
                data = realloc_32(data, max_malloc + sizeof(T) + sizeof(T) * idx);
                max_malloc++;
            }
        }
        ((T*)data)[idx] = value;
    }

    template <typename T, int dim>
    void add_array(int idx, const T *values) {
        if ((idx + dim) >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc_32(sizeof(T) * (idx + dim));
                max_malloc = idx;//+=idx;
            } else {
                data = realloc_32(data, sizeof(T) * (idx + dim));
                max_malloc = (idx + dim);
            }
        }
        memcpy(&((T*)data)[idx], values, dim);
    }

    template <typename T>
    void init_scalar(int id) {
        if (id >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc_32(sizeof(T) + sizeof(T) * id);
                max_malloc++;
            } else {
                data = realloc_32(data, max_malloc + sizeof(T) + sizeof(T) * id);
                max_malloc++;
            }
        }
    }

    template <typename T, int dim>
    void init_array(int id) {
        if ((id + dim) >= max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc_32(sizeof(T) * (id + dim));
                max_malloc = id;
            } else {
                data = realloc_32(data, sizeof(T) * (id + dim));
                max_malloc = (id + dim);
            }
        }
    }

};

template <>
struct create_type<BaseField> {
    operator MStructType*() { // implicit conversion
        std::vector<MType *> mtypes;
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(new MPointerType(create_mtype_type())); // TODO this shouldn't actually be used
        mtypes.push_back(new MPointerType(MScalarType::get_char_type()));
        return new MStructType(mtype_struct, mtypes);
    }
};

// These are purely convenient wrappers for BaseField
template <typename T, int _dim1 = 0, int _dim2 = 0>
class Field : public BaseField {
public:

    Field() : BaseField(_dim1, _dim2, (create_type<T>(_dim1, _dim2))) { }
};

template <typename T>
class Field<T,0> : public BaseField {

public:

    Field() : BaseField(0, 0, create_type<T>()) { }

};

// user fields

// can I make these reference full fields under the hood?
// in llvm, when I see a field inst type, use its ID to index into the full
// data array

// llvm generate code for a FieldInst: replace the field with a pointer to the correct
// or, can I compile a user function and then get a handle to it and modify it
// create a full data array for each field in a struct
// when codegening the field
// or, I could literally just write my own front end

// llvm instruction iterators for instructions
// build up function
//
//template <typename T>
//class FieldInst<T,0> : public BaseField {
//public:
//
//    FieldInst() : BaseField(0, 0, create_type<T>()) { }
//
//};
//
//template <typename T, int _dim1 = 0, int _dim2 = 0>
//class FieldInst : public BaseField {
//public:
//
//    FieldInst() : BaseField(_dim1, _dim2, (create_type<T>(_dim1, _dim2))) { }
//
//};


// TODO there's nothing here to enforce that you should use a field corresponding to this SetElement. You could basically pass in any field.
class SetElement {
    int id;

public:

    SetElement(int id) : id(id) {  }

    int get_element_id() const;

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
    T *get(Field<T,dim1,dim2> *field) const {
        return field->template get_array<T,dim1,dim2>(dim1 * dim2 * id);
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

    /*
     * These things down here are for initializing data/space in fields outside of LLVM
     */

    // These aren't for user to call
    template <typename T>
    void init(Field<T, 0> *field, T value) {
        field->template add_scalar<T,0>(id, value); // calls the BaseField func
    }

    template <typename T, int dim>
    void init(Field<T, dim> *field, const T *values) {
        field->template add_array<T,dim>(id * dim, values);
    }

    // These aren't for user to call
    template <typename T>
    void init_blank(Field<T, 0> *field) {
        field->template init_scalar<T>(id); // calls the BaseField func
    }

    template <typename T, int dim>
    void init_blank(Field<T, dim> *field) {
        field->template init_array<T,dim>(id * dim);
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

class Relation {

    std::vector<BaseField *> fields;

public:

    void add(BaseField *field);

    std::vector<MType *> get_mtypes();

    std::vector<BaseField *> get_fields();

};

void init_set_element(JIT *jit);

extern "C" SetElement **create_setelements(int num_to_create);


#endif //MATCHIT_FIELD_H