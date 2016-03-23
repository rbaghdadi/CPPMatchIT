#ifndef MATCHIT_FIELD_H
#define MATCHIT_FIELD_H

#include "./MType.h"

class PipelineInput { };

class DebugField {
protected:
    int idx;
    void *data = nullptr;
    int f;
    int x;
    int x2;
    int x3;
    int x4;
    int x5;

public:

    DebugField(int wtf) : idx(999), f(wtf) { }

//    virtual ~DebugField () {}

    void inc() {
        idx++;
    }

    virtual int blah() = 0;
};

template <typename T>
class TemplatedDebug : public DebugField {
public:
    TemplatedDebug() : DebugField(818) { }

    int blah() {
        return 2;
    }
};

struct create_debug_type {
private:

public:

    operator MStructType*() { // implicit conversion
        std::vector<MType *> mtypes;
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(new MPointerType(MScalarType::get_char_type())); // TODO or should this be field_type->underlying_type?
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        return new MStructType(mtype_struct, mtypes);
    }


};

class BaseField {
protected:
    int idx;
    int cur_length;
    int max_malloc;
    int dim1;
    int dim2;
    int element_ctr;
    int dummy_field;
    MType *data_mtype; // type of the data stored in this BaseField
    void *data = nullptr;

public:

    BaseField(int dim1, int dim2, MType *data_mtype) : idx(0), cur_length(0), max_malloc(0), dim1(dim1), dim2(dim2),
                                    element_ctr(0), dummy_field(26), data_mtype(data_mtype) { }


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

    int get_idx() {
        return idx;
    }

    int get_dim1() {
        return dim1;
    }

    int get_dim2() {
        return dim2;
    }

    int get_dummy_field() {
        return dummy_field;
    }

    MType *get_data_mtype() {
        return data_mtype;
    }

    int get_and_increment_ctr() {
        return element_ctr++;
    }

    int get_cur_length() {
        return cur_length;
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

    // these aren't for the user to call. Only used when initializing outside of llvm

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
                max_malloc = idx;
            } else {
                data = realloc(data, sizeof(T) * (idx + dim));
                max_malloc = (idx + dim);
            }
        }
    }

};

struct create_field_type {
private:
    MType *field_type;

public:
    create_field_type(MType *field_type) : field_type(field_type) { }

    operator MStructType*() { // implicit conversion
        std::vector<MType *> mtypes;
        mtypes.push_back(MScalarType::get_int_type());
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

    /*
     * These things down here are for initializing data/space in fields outside of LLVM
     */

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
        field->template init_scalar<T>(id); // calls the BaseField func
    }

    template <typename T, int dim>
    void init_blank(Field<T, dim> *field) {
        id = field->get_and_increment_ctr();
        field->template init_array<T,dim>(id * dim);
    }

    template <typename T>
    void init_no_malloc(Field<T, 0> *field) {
        id = field->get_and_increment_ctr();
    }

    template <typename T, int dim>
    void init_no_malloc(Field<T, dim> *field) {
        id = field->get_and_increment_ctr();
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
            mtypes.push_back((*iter)->get_data_mtype());
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