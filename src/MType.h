//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MTYPE_H
#define MATCHIT_MTYPE_H

#include <iostream>
#include "llvm/IR/Type.h"
#include "./JIT.h"

/**
 * All the possible types available for users to use in their stages
 */
typedef enum {
    mtype_null, // 0
    mtype_void, // 1
    mtype_bool, // 2
    mtype_char, // 3
    mtype_short, // 4
    mtype_int, // 5
    mtype_long, // 6
    mtype_float, // 7
    mtype_double, // 8
    mtype_struct, // 9
    mtype_ptr, // 10
    mtype_element, // 11
    mtype_comparison_element, // 12
    mtype_file, // 13
    mtype_segments, // 14
    mtype_segment, // 15
    mtype_marray, // 16
    mtype_mmatrix, // 17
    mtype_wrapper_output // 18 -- the return struct that wraps a user return type
} mtype_code_t;

/*
 * MType
 */

class MType {
protected:

    mtype_code_t mtype_code;

    unsigned int bits;

    std::vector<MType *> underlying_types;

    MType(mtype_code_t mtype_code) : mtype_code(mtype_code) {
        switch (mtype_code) {
            case mtype_bool:
                bits = 1;
            case mtype_char:
                bits = 8;
            case mtype_short:
                bits = 16;
            case mtype_int:
                bits = 32;
            case mtype_long:
                bits = 64;
            case mtype_float:
                bits = 32;
            case mtype_double:
                bits = 64;
            default:
                bits = 0;
        }
    }

    size_t sizeof_mtype(MType *type) {
        switch (type->get_mtype_code()) {
            case mtype_bool:
                return sizeof(bool);
            case mtype_char:
                return sizeof(char);
            case mtype_short:
                return sizeof(short);
            case mtype_int:
                return sizeof(int);
            case mtype_long:
                return sizeof(long);
            case mtype_float:
                return sizeof(float);
            case mtype_double:
                return sizeof(double);
            default:
                std::cerr << "bad user type for MType" << type->get_mtype_code() << std::endl;
                exit(8);
        }
    }

    MType(mtype_code_t mtype_code, unsigned int bits) : mtype_code(mtype_code), bits(bits) {}

public:

    MType() {}

    virtual ~MType() {}

    /**
     * Get the mtype_code_t for this MType
     */
    mtype_code_t get_mtype_code();

    /**
     * Get the number of bits in this MType
     */
    virtual unsigned int get_bits();

    /**
     * Generate LLVM code for this MType
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

    void add_underlying_type(MType *mtype);

    /**
     * Is this an MScalarType
     */
    bool is_prim_type();

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
     * Is this an mtype_mvector
     */
    bool is_mtype_marray_type();

    /**
     * Is this an mtype_file
     */
    bool is_mtype_file_type();

    /**
     * Is this an mtype_element
     */
    bool is_mtype_element_type();

    /**
     * Is this an mtype_segmented_element
     */
    bool is_mtype_segmented_element_type();

    /**
     * Is this an mtype_segments
     */
    bool is_mtype_segments_type();

    /**
     * Is this an mtype_int
     */
    bool is_mtype_comparison_element_type();

    /**
     * Is this an mtype_stage
     */
    bool is_mtype_stage();

    /**
     * Print out all the types associated with this MType
     */
    virtual void dump() = 0;

    /**
     * Override the number of bits in this MType
     */
    void set_bits(unsigned int bits);

    virtual MType* get_user_type() { std::cerr << "wtf" << std::endl; return nullptr; }

    virtual size_t get_size() = 0;

};

/*
 * MScalarType
 */

class MScalarType : public MType {
private:

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
        assert(is_prim_type());
    }

    ~MScalarType() {}

    llvm::Type *codegen_type();

    void dump();

    size_t get_size() {
        return bits / 8;
    }

    size_t _sizeof_T_type() {
        return bits / 8;
    }

    size_t _sizeof_ptr() {
        return 8;
    }

    unsigned int get_bits() {
        return bits;
    }

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

class MPointerType : public MType {
public:

    MPointerType() {}

    MPointerType(MType *pointer_type) : MType(mtype_ptr, 64) {
        underlying_types.push_back(pointer_type);
    }

    ~MPointerType() {}

    llvm::Type *codegen_type();

    void dump();

    size_t _sizeof() {
        return 8;
    }

    size_t get_size() {
        return 8;
    }

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

    void dump();

    llvm::Type *codegen_type();

    size_t _sizeof() {
        size_t total = 0;
        for (std::vector<MType *>::iterator iter = underlying_types.begin(); iter != underlying_types.end(); iter++) {
            total += (*iter)->get_size();//sizeof(*iter);
        }
        return total;
    }

    size_t get_size() {
        return _sizeof();
    }

};

struct create_mtype_type {

    operator MStructType*() {
        std::vector<MType *> mtypes;
        mtypes.push_back(MScalarType::get_int_type());
        mtypes.push_back(MScalarType::get_int_type());
        return new MStructType(mtype_struct, mtypes);
    }

};

class MArrayType : public MType {
private:

    int length;
    bool variable_length;
    MType *array_element_type;

public:

    MArrayType(int length, MType *array_element_type) : length(length), array_element_type(array_element_type) {
//        if (length == 0) {
//            variable_length = true;
//        } else {
        variable_length = false;
//        }
        underlying_types.push_back(array_element_type);
        set_bits(array_element_type->get_size() * 8);
        mtype_code = mtype_marray;
    }

    int get_length() {
        return length;
    }

    MType *get_array_element_type() {
        return array_element_type;
    }

    size_t get_size() {
        return (array_element_type->get_bits() / 8) * length;
    }

    void dump() {
        std::cerr << "MArrayType" << std::endl;
    }

    llvm::Type *codegen_type() {
        std::vector<MType *> types;
        types.push_back(new MPointerType(array_element_type)); // data
        types.push_back(MScalarType::get_int_type()); // size
        MStructType s(mtype_struct, types);
        return s.codegen_type();
    }

    bool is_variable_length() {
        return variable_length;
    }
};

class MMatrixType : public MType {
private:

    int row_dimension;
    int col_dimension;
    bool x_variable_length;
    bool y_variable_length;
    MType *matrix_element_type;

public:

    MMatrixType(int row_dimension, int col_dimension, MType *matrix_element_type) :
            row_dimension(row_dimension), col_dimension(col_dimension), matrix_element_type(matrix_element_type) {
//        if (row_dimension == 0) {
//            x_variable_length = true;
//        } else if (col_dimension == 0){
//            y_variable_length = true;
//        } else {
//            x_variable_length = false;
//            y_variable_length = false;
//        }
        underlying_types.push_back(matrix_element_type);
        set_bits(matrix_element_type->get_size() * 8);
        mtype_code = mtype_mmatrix;
    }

    int get_length() {
        return row_dimension * col_dimension;
    }

    int get_row_dimension() {
        return row_dimension;
    }

    int get_col_dimension() {
        return col_dimension;
    }

    MType *get_array_element_type() {
        return matrix_element_type;
    }

    size_t get_size() {
        return (matrix_element_type->get_bits() / 8);// * (row_dimension * col_dimension);
    }

    void dump() {
        std::cerr << "MMatrixType" << std::endl;
    }

    llvm::Type *codegen_type() {
        assert(false); // TODO not done yet
        return nullptr;
    }

    bool is_variable_length() {
        return x_variable_length;
    }
};

template <typename T>
struct create_scalar_type;

template <typename T>//, int row_dimension>
struct create_array_type;

template <typename T>//, int row_dimension, int col_dimension>
struct create_matrix_type;

template <typename T>//, int row_dimension = 1, int col_dimension = 0>
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
            return (MArrayType*)create_array_type<T>(row_dimension); // if row_dimension == 0, we will have a variable row_dimension array
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
        return MScalarType::get_bool_type(); // this is a custom implicit conversion
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

template <typename T>
struct mtype_of {
    operator mtype_code_t() {
        return mtype_struct;
    }
};

template <typename T>
struct mtype_of<T *> {
    operator mtype_code_t() {
        return mtype_ptr;
    }
};

template <>
struct mtype_of<bool> {
    operator mtype_code_t() {
        return mtype_bool;
    }
};

template <>
struct mtype_of<char> {
    operator mtype_code_t() {
        return mtype_char;
    }
};

template <>
struct mtype_of<short> {
    operator mtype_code_t() {
        return mtype_short;
    }
};

template <>
struct mtype_of<int> {
    operator mtype_code_t() {
        return mtype_int;
    }
};

template <>
struct mtype_of<long> {
    operator mtype_code_t() {
        return mtype_long;
    }
};

template <>
struct mtype_of<float> {
    operator mtype_code_t() {
        return mtype_float;
    }
};

template <>
struct mtype_of<double> {
    operator mtype_code_t() {
        return mtype_double;
    }
};

template <>
struct mtype_of<void> {
    operator mtype_code_t() {
        return mtype_void;
    }
};

#endif //MATCHIT_MTYPE_H
