//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MTYPE_H
#define MATCHIT_MTYPE_H

#include <iostream>
#include "llvm/IR/Type.h"
#include "./JIT.h"

typedef enum {
    mtype_null,
    mtype_void,
    mtype_bool,
    mtype_char,
    mtype_short,
    mtype_int,
    mtype_long,
    mtype_float,
    mtype_double,
    mtype_struct,
    mtype_ptr,
    mtype_marray,
    mtype_mmatrix
} mtype_code_t;

/*
 * MType
 */

class MType {
protected:

    /**
     * Type code corresponding to this MType
     */
    mtype_code_t mtype_code;

    /**
     * If the C type is T, this is essentially the result of calling sizeof(T)
     */
    unsigned int bytes;

    /**
     * If this is a "composite" type (struct, pointer, array, or matrix), this is the element type (or types in
     * the case of a struct).
     */
    std::vector<MType *> underlying_types;

    MType(mtype_code_t mtype_code, unsigned int bytes) : mtype_code(mtype_code), bytes(bytes) { }

public:

    MType() {}

    virtual ~MType() {}

    /**
     * Get the mtype_code_t for this MType.
     */
    mtype_code_t get_mtype_code();

    /**
     * Generate LLVM code for this MType.
     */
    virtual llvm::Type *codegen_type() = 0;

    /**
     * Get the types underlying this current MType.
     * For example, if this type is an MPointerType,
     * this would return the type that the pointer points to.
     * If this in an MStructType, this would return the MTypes
     * of all the fields that make up the struct.
     */
    std::vector<MType *> get_underlying_types();

    /**
     * Add an underlying type to the underlying_types vector.
     */
    void add_underlying_type(MType *mtype);

    /**
     * Is this a primitive (MScalarType)
     */
    bool is_scalar_type();

    /**
     * Is this a signed type
     */
    bool is_signed_type();

    /**
     * Is this a signed type
     */
    bool is_unsigned_type() {
        assert(false && "not an unsigned type");
    }

    /**
     * Is this an mtype_bool
     */
    bool is_bool_type();

    /**
     * Is this an mtype_int
     */
    bool is_int_type();

    /**
     * Is this an mtype_float
     */
    bool is_float_type();

    /**
     * Is this an mtype_double
     */
    bool is_double_type();

    /**
     * Is this an mtype_void
     */
    bool is_void_type();

    /**
     * Is this an mtype_struct
     */
    bool is_struct_type();

    /**
     * Is this an mtype_ptr
     */
    bool is_ptr_type();

    /**
     * Is this an mtype_marray
     */
    bool is_mtype_marray_type();

    /**
     * Print out all the types associated with this MType.
     */
    virtual void dump() = 0;

    /**
     * Get size of the underlying types of this MType.
     */
    virtual int underlying_size() = 0;

    virtual bool is_mscalar_type();

    virtual bool is_mpointer_type();

    virtual bool is_mstruct_type();

    virtual bool is_marray_type();

    virtual bool is_mmatrix_type();

};

/*
 * MScalarType
 */

class MScalarType : public MType {
private:

    // premade types
    static MScalarType *void_type;
    static MScalarType *bool_type;
    static MScalarType *char_type;
    static MScalarType *short_type;
    static MScalarType *int_type;
    static MScalarType *long_type;
    static MScalarType *float_type;
    static MScalarType *double_type;

public:

    MScalarType(mtype_code_t mtype_code, unsigned int bits) : MType(mtype_code, bits) {
        assert(is_scalar_type());
    }

    ~MScalarType() {}

    /**
     * Generate simple LLVM Type (float/double) or IntegerType (void, bool, char, short, int, long)
     */
    llvm::Type *codegen_type();

    /**
     * Print out the type of this scalar.
     */
    void dump();

    /**
     * Return the number of bytes of this scalar value.
     */
    int underlying_size();

    bool is_mscalar_type();

    /**
     * Get preconstructed mtype_bool
     */
    static MScalarType *get_bool_type();

    /**
     * Get preconstructed mtype_char
     */
    static MScalarType *get_char_type();

    /**
     * Get preconstructed mtype_short
     */
    static MScalarType *get_short_type();

    /**
     * Get preconstructed mtype_int
     */
    static MScalarType *get_int_type();

    /**
     * Get preconstructed mtype_long
     */
    static MScalarType *get_long_type();

    /**
     * Get preconstructed mtype_float
     */
    static MScalarType *get_float_type();

    /**
     * Get preconstructed mtype_double
     */
    static MScalarType *get_double_type();

    /**
     * Get preconstructed mtype_void
     */
    static MScalarType *get_void_type();

};

/*
 * MPointerType
 */

class MPointerType : public MType {
public:

    MPointerType() {}

    MPointerType(MType *pointer_type) : MType(mtype_ptr, 64) {
        underlying_types.push_back(pointer_type);
    }

    ~MPointerType() {}

    /**
     * Generate LLVM PointerType.
     */
    llvm::Type *codegen_type();

    /**
     * Print out the element type that this pointer points to.
     */
    void dump();

    /**
     * Return the number of bytes in the element type that this pointer points to.
     */
    int underlying_size();

    bool is_mpointer_type();

};

/*
 * MStructType
 */

class MStructType : public MType {
public:

    MStructType(mtype_code_t mtype_code) : MType(mtype_code, 0) {}

    MStructType(mtype_code_t mtype_code, std::vector<MType *> underlying_types) : MType(mtype_code, 0) {
        for (std::vector<MType *>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
            this->underlying_types.push_back(*iter);
        }
    }

    /**
     * Print out the invidual element types of this MStruct.
     */
    void dump();

    /**
     * Generate an LLVM StructType.
     */
    llvm::Type *codegen_type();

    /**
     * Sum up the number of bytes in each of the underlying types.
     */
    int underlying_size();

    bool is_mstruct_type();

};

/**
 * Creates a type that is the actual MType type. Hack for BaseField.
 */
struct create_mtype_type {

    operator MStructType*() {
        std::vector<MType *> mtypes;
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(new MPointerType(MScalarType::get_char_type())); // TODO need a delete for this
        return new MStructType(mtype_struct, mtypes);
    }

};

/*
 * MarrayType
 */

class MArrayType : public MType {
private:

    /**
     * Length of this array.
     */
    int length;

    /**
     * Type of the array element.
     */
    MType *array_element_type;

public:

    MArrayType(int length, MType *array_element_type) : length(length), array_element_type(array_element_type) {
        underlying_types.push_back(array_element_type);
        bytes = array_element_type->underlying_size();
        mtype_code = mtype_marray;
    }

    /**
     * Get the length of this array.
     */
    int get_length();

    /**
     * Get the type of the array element.
     */
    MType *get_array_element_type();

    /**
     * Print out the type of the array element.
     */
    void dump();

    /**
     * TODO not used. I think this is because the actual array type is abstracted away from llvm due to using the fields.
     */
    llvm::Type *codegen_type();

    /**
     * Return the number of bytes in the array element type.
     */
    int underlying_size();

    bool is_marray_type();

};

class MMatrixType : public MType {
private:

    int row_dimension;
    int col_dimension;
    MType *matrix_element_type;

public:

    MMatrixType(int row_dimension, int col_dimension, MType *matrix_element_type) :
            row_dimension(row_dimension), col_dimension(col_dimension), matrix_element_type(matrix_element_type) {
        underlying_types.push_back(matrix_element_type);
        bytes = matrix_element_type->underlying_size();
        mtype_code = mtype_mmatrix;
    }

    /**
     * Number of rows in this matrix.
     */
    int get_row_dimension();

    /**
     * Number of columns in this matrix.
     */
    int get_col_dimension();

    /**
     * Get the type of the matrix element.
     */
    MType *get_matrix_element_type();

    /**
     * Print out the matrix element type.
     */
    void dump();

    /**
     * TODO not used. See MArrayType
     */
    llvm::Type *codegen_type();

    /**
     * Return the number of bytes in the matrix element type.
     */
    int underlying_size();

    bool is_mmatrix_type();

};

/*
 * Builders for the types.
 */

template <typename T>
struct create_scalar_type;

template <typename T>
struct create_array_type;

template <typename T>
struct create_matrix_type;

template <typename T>
struct create_type {
    int row_dimension;
    int col_dimension;

    create_type() : row_dimension(0), col_dimension(0) { }
    create_type(int row_dimension) : row_dimension(row_dimension), col_dimension(0) { }
    create_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MType*() { // implicit conversion
        if (row_dimension == 1 && col_dimension == 0) {
            return (MScalarType*)create_scalar_type<T>();
        } else if (col_dimension == 0){
            return (MArrayType*)create_array_type<T>(row_dimension);
        } else {
            return (MMatrixType*)create_matrix_type<T>(row_dimension, col_dimension);
        }
    }
};

/*
 * create_scalar_type
 */

template <>
struct create_scalar_type<bool> {
    operator MScalarType*() {
        return MScalarType::get_bool_type();
    }
};

template <>
struct create_scalar_type<char> {
    operator MScalarType*() {
        return MScalarType::get_char_type();
    }
};

template <>
struct create_scalar_type<unsigned char> {
    operator MScalarType*() {
        return MScalarType::get_char_type();
    }
};

template <>
struct create_scalar_type<short> {
    operator MScalarType*() {
        return MScalarType::get_short_type();
    }
};

template <>
struct create_scalar_type<int> {
    operator MScalarType*() {
        return MScalarType::get_int_type();
    }
};

template <>
struct create_scalar_type<unsigned int> {
    operator MScalarType*() {
        return MScalarType::get_int_type();
    }
};

template <>
struct create_scalar_type<long> {
    operator MScalarType*() {
        return MScalarType::get_long_type();
    }
};

template <>
struct create_scalar_type<float> {
    operator MScalarType*() {
        return MScalarType::get_float_type();
    }
};

template <>
struct create_scalar_type<double> {
    operator MScalarType*() {
        return MScalarType::get_double_type();
    }
};

/*
 * create array type
 */

template <>
struct create_array_type<bool> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<bool>());
    }
};

template <>
struct create_array_type<char> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<char>());
    }
};

template <>
struct create_array_type<unsigned char> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<unsigned char>());
    }
};

template <>
struct create_array_type<short> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<short>());
    }
};

template <>
struct create_array_type<int> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<int>());
    }
};

template <>
struct create_array_type<unsigned int> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<unsigned int>());
    }
};

template <>
struct create_array_type<long> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<long>());
    }
};

template <>
struct create_array_type<float> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<float>());
    }
};

template <>
struct create_array_type<double> {
    int array_size;
    create_array_type(int array_size) : array_size(array_size) { }

    operator MArrayType*() {
        return new MArrayType(array_size, create_scalar_type<double>());
    }
};

/*
 * create_matrix_type
 */

template <>
struct create_matrix_type<bool> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<bool>());
    }
};

template <>
struct create_matrix_type<char> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<char>());
    }
};

template <>
struct create_matrix_type<unsigned char> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<unsigned char>());
    }
};

template <>
struct create_matrix_type<short> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<short>());
    }
};

template <>
struct create_matrix_type<int> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<int>());
    }
};

template <>
struct create_matrix_type<unsigned int> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<unsigned int>());
    }
};

template <>
struct create_matrix_type<long> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<long>());
    }
};

template <>
struct create_matrix_type<float> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<float>());
    }
};

template <>
struct create_matrix_type<double> {
    int row_dimension;
    int col_dimension;
    create_matrix_type(int row_dimension, int col_dimension) : row_dimension(row_dimension), col_dimension(col_dimension) { }

    operator MMatrixType*() {
        return new MMatrixType(row_dimension, col_dimension, create_scalar_type<double>());
    }
};

#endif //MATCHIT_MTYPE_H
