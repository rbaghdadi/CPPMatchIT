#ifndef MATCHIT_FIELD_H
#define MATCHIT_FIELD_H

#include "./MType.h"
#include "./Utils.h"

// DON'T ADD VIRTUAL METHODS TO THIS OTHERWISE IT WILL BREAK!!!!
// scalar field: dim1 = 0 && dim2 = 0
// array field: dim1 != 0 && dim2 = 0
// matrix field: dim1 != 0 && dim2 != 0
class BaseField {
protected:
    int cur_length;
    int max_malloc;
    int dim1;
    int dim2;
    int element_ctr;
    MType *data_mtype; // type of the data stored in this BaseField
    void *data = nullptr;

public:

    BaseField(int dim1, int dim2, MType *data_mtype) : cur_length(0), max_malloc(0), dim1(dim1), dim2(dim2),
                                                       element_ctr(0), data_mtype(data_mtype) { }

    int get_dim1();

    int get_dim2();

    int get_dummy_field();

    MType *get_data_mtype();

    int get_and_increment_ctr();

    int get_cur_length();

    static int get_data_idx();

    // size for a single entry in a field (i.e. for a single Element)
    int get_fixed_size();

    template <typename T>
    T get_scalar(int element_idx) {
        return ((T*)data)[element_idx];
    }

    template <typename T>
    T get_array_scalar(int element_idx, int offset) {
        return ((T*)data)[element_idx * dim1 + offset];
    }

    template <typename T>
    T *get_array(int element_idx) {
        return &((T*)data)[element_idx * dim1];
    }

    template <typename T>
    T get_matrix_scalar(int element_idx, int row_offset, int col_offset) {
        return ((T*)data)[element_idx * dim1 * dim2 + row_offset * dim1 + col_offset];
    }

    template <typename T>
    T *get_matrix_array(int element_idx, int row_offset) {
        return &((T*)data)[element_idx * dim1 * dim2 + row_offset * dim1];
    }

    /**
     * Add an individual element to a scalar Field
     */
    template <typename T>
    void set_scalar(int element_idx, T scalar) {
        ((T*)data)[element_idx] = scalar;
    }

    /**
     * Add an individual element to an array Field
     */
    template <typename T>
    void set_array_scalar(int element_idx, int offset, T scalar) {
        ((T*)data)[element_idx * dim1 + offset] = scalar;
    }

    /**
     * Add an individual element to a matrix Field
     */
    template <typename T>
    void set_matrix_scalar(int element_idx, int row_offset, int col_offset, T scalar) {
        ((T*)data)[element_idx * dim1 * dim2 + dim1 * row_offset + col_offset] = scalar;
    }

    /**
     * Add an array element to an array Field
     */
    template <typename T>
    void set_array(int element_idx, T *array) {
        memcpy(&((T*)data)[element_idx * dim1], array, dim1);
    }

    /**
     * Add an array element to a matrix Field
     */
    template <typename T>
    void set_matrix_row(int element_idx, int row_offset, T *array) {
        memcpy(&((T*)data)[element_idx * dim1 * dim2 + row_offset * dim1], array, dim1);
    }

    /**
     * Allocate space for a scalar Field
     */
    template <typename T>
    void scalar_allocate_only(int element_idx) {
        if (element_idx+1 > max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc_32(sizeof(T) + sizeof(T) * element_idx);
            } else {
                data = realloc_32(data, sizeof(T) + sizeof(T) * element_idx);
            }
            max_malloc = element_idx + 1;
        }
    }

    /**
     * Allocate space for an array Field
     */
    template <typename T>
    void array_allocate_only(int element_idx) {
        int starting_idx = element_idx * dim1;
        if (starting_idx + dim1 > max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc_32(sizeof(T) * (starting_idx + dim1));
            } else {
                data = realloc_32(data, sizeof(T) * (starting_idx + dim1));
            }
            max_malloc = starting_idx + dim1;
        }
    }

    /**
     * Allocate space for a matrix Field
     */
    template <typename T>
    void matrix_allocate_only(int element_idx) {
        int starting_idx = element_idx * dim1 * dim2;
        if (starting_idx + dim1 * dim2 > max_malloc) {
            if (!data) { // hasn't been mallocd yet
                data = malloc_32(sizeof(T) * (starting_idx + dim1 * dim2));
            } else {
                data = realloc_32(data, sizeof(T) * (starting_idx + dim1 * dim2));
            }
            max_malloc = starting_idx + dim1 * dim2;
        }
    }

    /**
     * Allocate space for a scalar Field (if necessary) and set value
     */
    template <typename T>
    void scalar_allocate_and_set(int element_idx, T scalar) {
        scalar_allocate_only<T>(element_idx);
        set_scalar(element_idx, scalar);
    }

    /**
     * Allocate space for an array Field (if necessary) and set value
     */
    template <typename T>
    void array_allocate_and_set(int element_idx, T *array) {
        array_allocate_only<T>(element_idx);
        set_array(element_idx, array);
    }

    /**
     * Allocate space for an array Field (if necessary) and set individual scalar of the array
     */
    template <typename T>
    void array_allocate_and_set_scalar(int element_idx, int offset, T scalar) {
        array_allocate_only<T>(element_idx);
        set_array_scalar(element_idx, offset, scalar);
    }

    /**
     * Allocate space for a matrix Field (if necessary) and set value
     */
    template <typename T>
    void matrix_allocate_and_set(int element_idx, T **matrix) {
        matrix_allocate_only<T>(element_idx);
        for (int i = 0; i < dim2; i++) { // iterate down columns
            set_matrix_row(element_idx, i, matrix[i]);
        }
    }

    /**
     * Allocate space for a matrix Field (if necessary) and set a row of the matrix
     */
    template <typename T>
    void matrix_allocate_and_set_array(int element_idx, int row_offset, T *array) {
        matrix_allocate_only<T>(element_idx);
        set_matrix_row(element_idx, row_offset, array);
    }

    /**
     * Allocate space for a matrix Field (if necessary) and set an individual scalar of the matrix
     */
    template <typename T>
    void matrix_allocate_and_set_scalar(int element_idx, int row_offset, int col_offset, T scalar) {
        matrix_allocate_only<T>(element_idx);
        set_matrix_scalar(element_idx, row_offset, col_offset, scalar);
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

// TODO there's nothing here to enforce that you should use a field corresponding to this Element. You could basically pass in any field.
class Element {
    int id;

public:

    Element(int id) : id(id) {  }

    int get_element_id() const;

    /**
     * Get element from scalar Field
     */
    template <typename T>
    T get(Field<T, 0, 0> *field) const {
        return field->template get_scalar<T>(id);
    }

    /**
     * Get array from an array Field
     */
    template <typename T, int dim>
    T *get(Field<T, dim> *field) const {
        return field->template get_array<T>(id);
    }

    /**
     * Get scalar from an array Field
     */
    template <typename T, int dim>
    T get(Field<T, dim> *field, int offset) const {
        return field->template get_array_scalar<T>(id, offset);
    }

    /**
     * Get row from a matrix Field
     */
    template <typename T, int dim1, int dim2>
    T *get(Field<T,dim1,dim2> *field, int row_offset) const {
        return field->template get_matrix_array<T>(id, row_offset);
    }

    /**
     * Get scalar from a matrix Field.
     */
    template <typename T, int dim1, int dim2>
    T get(Field<T,dim1,dim2> *field, int row_offset, int col_offset) const {
        return field->template get_matrix_scalar<T>(id, row_offset, col_offset);
    }

    /*
     * Setters for the user functions
     */

    /**
     * Set value in a scalar Field.
     */
    template <typename T>
    void set(Field<T, 0> *field, T val) {
        field->template set_scalar<T>(id, val);
    }

    /**
     * Set single value in an array Field.
     */
    template <typename T, int dim>
    void set(Field<T, dim> *field, int offset, T val) {
        field->template set_array_scalar<T>(id, offset, val);
    }

    /**
     * Set array in an array Field.
     */
    template <typename T, int dim>
    void set(Field<T, dim> *field, T *vals) {
        field->template set_array<T>(id, vals);
    }

    /**
     * Set single value in a matrix Field.
     */
    template <typename T, int dim1, int dim2>
    void set(Field<T, dim1, dim2> *field, int row_offset, int col_offset, T val) {
        field->template set_matrix_scalar<T>(id, row_offset, col_offset, val);
    }

    /**
     * Set row in a matrix Field.
     */
    template <typename T, int dim1, int dim2>
    void set(Field<T, dim1, dim2> *field, int row_offset, T *vals) {
        field->template set_matrix_row<T>(id, row_offset, vals);
    }

    /*
     * Setters for initialization.
     */


    /**
     * Allocate space for a scalar Field.
     */
    template <typename T>
    void allocate(Field<T, 0, 0> *field) {
        field->template scalar_allocate_only<T>(id);
    }

    /**
     * Allocate space for an array Field.
     */
    template <typename T, int dim>
    void allocate(Field<T, dim> *field) {
        field->template array_allocate_only<T>(id);
    }

    /**
     * Allocate space for a matrix Field.
     */
    template <typename T, int dim1, int dim2>
    void allocate(Field<T, dim1, dim2> *field) {
        field->template matrix_allocate_only<T>(id);
    }

    /**
     * Allocate space for a scalar Field and set value.
     */
    template <typename T>
    void allocate_and_set(Field<T, 0> *field, T val) {
        field->template scalar_allocate_and_set<T>(id, val);
    }

    /**
     * Allocate space for an array Field and set a scalar.
     */
    template <typename T, int dim>
    void allocate_and_set(Field<T, dim> *field, int offset, T val) {
        field->template array_allocate_and_set_scalar<T>(id, offset, val);
    }

    /**
     * Alocate space for an array Field and set an array.
     */
    template <typename T, int dim>
    void allocate_and_set(Field<T, dim> *field, T *vals) {
        field->template array_allocate_and_set<T>(id, vals);
    }

    /**
     * Allocate space for a matrix Field and set a scalar.
     */
    template <typename T, int dim1, int dim2>
    void allocate_and_set(Field<T, dim1, dim2> *field, int row_offset, int col_offset, T val) {
        field->template matrix_allocate_and_set_scalar(id, row_offset, col_offset, val);
    }

    /**
    * Allocate space for a matrix Field and set an array.
    */
    template <typename T, int dim1, int dim2>
    void allocate_and_set(Field<T, dim1, dim2> *field, int row_offset, T *vals) {
        field->template matrix_allocate_and_set_array<T>(id, row_offset, vals);
    }



};

template <>
struct create_type<Element> {
    operator MStructType*() {
        std::vector<MType *> mtype;
        mtype.push_back(MScalarType::get_int_type());
        return new MStructType(mtype_struct, mtype);
    }
};

class Fields {

    std::vector<BaseField *> fields;

public:

    void add(BaseField *field);

    std::vector<MType *> get_mtypes();

    std::vector<BaseField *> get_fields();

};

void init_element(JIT *jit);

extern "C" Element **create_elements(int num_to_create);


#endif //MATCHIT_FIELD_H