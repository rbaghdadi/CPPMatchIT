//
// Created by Jessica Ray on 1/28/16.
//


#include <map>
#include <iostream>
#include "llvm/IR/Type.h"

#ifndef MATCHIT_MTYPE_H
#define MATCHIT_MTYPE_H

class MType;
class MPrimType;
class MStructType;
class MPointerType;

// eventually we would need all of the standard types...I think
// TODO unsigned types? I don't think LLVM differentiates between the two
typedef enum {
    mtype_null, // just for initial construction. not used for any actual type
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
    mtype_class
} mtype_code_t;

// modeled after halide
// need structs to get around the limitation of float and double as type parameters
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

// MType

// TODO I probably don't need to create a new instance of this each time I use it because it all kind of stays the same
class MType {

protected:
    mtype_code_t type_code;
    unsigned int bits;

//    static const std::map<mtype_code_t, unsigned int> bits_map = {{mtype_void,0}, {mtype_char,8}, {mtype_short,16},
//                                                            {mtype_int, 32}, {mtype_long, 64}, {mtype_float,32},
//                                                            {mtype_double,64}, {mtype_struct, 0}};

    MType(mtype_code_t type_code) : type_code(type_code) {
        switch (type_code) {
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
            case mtype_struct:
                bits = 0;
        }
    }

    MType(mtype_code_t type_code, unsigned int bits) : type_code(type_code), bits(bits) {}

public:

    static MType *bool_type;
    static MType *char_type;
    static MType *short_type;
    static MType *int_type;
    static MType *long_type;
    static MType *float_type;
    static MType *double_type;

    MType() {}

    ~MType() {}

    mtype_code_t get_type_code();

    unsigned int get_bits();

    virtual unsigned int get_alignment();

    void set_bits(unsigned int bits);

    virtual llvm::Type *codegen() =0;

//    virtual llvm::Type *codegen_constant_array() =0;

    bool is_prim_type();

    bool is_bool_type();

    bool is_int_type();

    bool is_float_type();

    bool is_double_type();

    bool is_struct_type();

    bool is_ptr_type();

};


/*
 * MPrimType
 */

class MPrimType : public MType {

public:

    MPrimType(mtype_code_t type_code, unsigned int bits) : MType(type_code, bits) {
        assert(is_prim_type());
    }

    unsigned int get_alignment();

    llvm::Type *codegen();

//    llvm::Type *codegen_constant_array();

};

/*
 * MStructType
 */

class MStructType : public MType {

    std::vector<MType *> field_types;

public:

    // constructs an empty struct type
    MStructType() {}

    MStructType(mtype_code_t type_code, std::vector<MType *> field_types) : MType(type_code, 0), field_types(field_types) {
        assert(is_struct_type());
        unsigned int num_bits = 0;
        for (std::vector<MType*>::iterator iter = field_types.begin(); iter != field_types.end(); iter++) {
            num_bits += (*iter)->get_bits();
        }
        set_bits(num_bits);
    }

    unsigned int get_alignment();

    llvm::Type *codegen();

//    llvm::Type *codegen_constant_array();

};

/*
 * MPointerType
 */

// TODO this will break if you try to make a pointer of a non-primitive (so like pointer of structs)
class MPointerType : public MType {

private:

//    mtype_code_t pointer_type_code; // says what is this pointer pointing to
    MType *pointer_type;

public:

    MPointerType() {}

    // this is bits of the pointer_type_code
    MPointerType(MType *pointer_type) : MType(pointer_type->get_type_code(), 64), pointer_type(pointer_type) { // TODO 64 is platform specific
    }

    unsigned int get_alignment();

    llvm::Type *codegen();

//    llvm::Type *codegen_constant_array();

};

/*
 * Premade types
 */



namespace {

    template <typename T>
    struct create_type;

    template <>
    struct create_type<bool> {
        operator MType*() {
            return MType::bool_type;
        }
    };

    template <>
    struct create_type<char> {
        operator MType*() {
            return MType::char_type;
        }
    };

    template <>
    struct create_type<unsigned char> {
        operator MType*() {
            return MType::char_type;
        }
    };

    template <>
    struct create_type<short> {
        operator MType*() {
            return MType::short_type;
        }
    };

    template <>
    struct create_type<int> {
        operator MType*() {
            return MType::int_type;
        }
    };

    template <>
    struct create_type<long> {
        operator MType*() {
            return MType::long_type;
        }
    };

    template <>
    struct create_type<float> {
        operator MType*() {
            return MType::float_type;
        }
    };

    template <>
    struct create_type<double> {
        operator MType*() {
            return MType::double_type;
        }
    };

    // a user type. needs to be a struct for now (class maybe works?)
    template <typename T>
    struct create_type {
        operator MType*() {
            std::cerr << "MType.h: Shouldn't be in here (create_type for typename T)!" << std::endl;
            exit(1);

        }
    };

    // TODO don't use new unless I can make sure I delete it
    template <typename T>
    struct create_type<T *> {
        operator MPointerType*() {
            MPointerType *ptr = new MPointerType(create_type<T>());
            return ptr;
        }
    };

    MStructType *create_struct_type(std::vector<MType *> field_types) {
        return new MStructType(mtype_struct, field_types);
    }

    MPointerType *create_struct_reference_type(std::vector<MType *> field_types) {
        return new MPointerType(create_struct_type(field_types));
    }

}


#endif //MATCHIT_MTYPE_H
