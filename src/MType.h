//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_MTYPE_H
#define MATCHIT_MTYPE_H

#include <iostream>
#include "llvm/IR/Type.h"
#include "./Structures.h"
#include "./JIT.h"

// TODO does everything work if the user returns a single type like an mtype_int?
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
    mtype_segmented_element, // 15
    mtype_marray, // 16
    mtype_wrapper_output // 17 -- the return struct that wraps a user return type
} mtype_code_t;

/*
 * MType
 */

class MType {
private:

    /*
     * Premade types
     */

    static MType *void_type;
    static MType *bool_type;
    static MType *char_type;
    static MType *short_type;
    static MType *int_type;
    static MType *long_type;
    static MType *float_type;
    static MType *double_type;

protected:

    mtype_code_t mtype_code;

    std::vector<MType *> underlying_types;

    unsigned int bits;

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
    unsigned int get_bits();

    /**
     * Generate LLVM code for this MType
     */
    virtual llvm::Type *codegen_type() = 0;

    virtual size_t _sizeof() = 0;

//    virtual llvm::Value *codegen_pool(JIT *jit, int num_elements) = 0;

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
     * Is this an MPrimType
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
     * Get preconstructed mtype_bool
     */
    static MType *get_bool_type();

    /**
    * Get preconstructed mtype_char
    */
    static MType *get_char_type();

    /**
    * Get preconstructed mtype_short
    */
    static MType *get_short_type();

    /**
    * Get preconstructed mtype_int
    */
    static MType *get_int_type();

    /**
    * Get preconstructed mtype_long
    */
    static MType *get_long_type();

    /**
    * Get preconstructed mtype_float
    */
    static MType *get_float_type();

    /**
    * Get preconstructed mtype_double
    */
    static MType *get_double_type();

    /**
    * Get preconstructed mtype_void
    */
    static MType *get_void_type();

    /**
     * Print out all the types associated with this MType
     */
    virtual void dump() = 0;

    /**
     * Override the number of bits in this MType
     */
    void set_bits(unsigned int bits);

//    virtual size_t _sizeof() = 0;

    virtual llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                                 llvm::Function *function) = 0;

    virtual llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length,
                                                 llvm::Function *function) = 0;

};

/*
 * MPrimType
 */

class MPrimType : public MType {
public:

    MPrimType(mtype_code_t mtype_code, unsigned int bits) : MType(mtype_code, bits) {
        assert(is_prim_type());
    }

    ~MPrimType() {}

    llvm::Type *codegen_type();

    void dump();

    size_t _sizeof() {
        return bits / 8;
    }

    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }

};

class MPointerType : public MType {
public:

    MPointerType() {}

    MPointerType(MType *pointer_type) : MType(mtype_ptr, 64) {
        underlying_types.push_back(pointer_type);
    } // TODO 64 is platform specific

    ~MPointerType() {}

    llvm::Type *codegen_type();

    void dump();

    size_t _sizeof() {
        return 8;
    }

    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }

};

template <typename T>
struct create_type;

template <>
struct create_type<bool> {
    operator MType*() {
        return MType::get_bool_type();
    }
};

template <>
struct create_type<char> {
    operator MType*() {
        return MType::get_char_type();
    }
};

template <>
struct create_type<unsigned char> {
    operator MType*() {
        return MType::get_char_type();
    }
};

template <>
struct create_type<short> {
    operator MType*() {
        return MType::get_short_type();
    }
};

template <>
struct create_type<int> {
    operator MType*() {
        return MType::get_int_type();
    }
};

template <>
struct create_type<unsigned int> {
    operator MType*() {
        return MType::get_int_type();
    }
};

template <>
struct create_type<long> {
    operator MType*() {
        return MType::get_long_type();
    }
};

template <>
struct create_type<float> {
    operator MType*() {
        return MType::get_float_type();
    }
};

template <>
struct create_type<double> {
    operator MType*() {
        return MType::get_double_type();
    }
};

template <typename T>
struct create_type<T *> {
    operator MPointerType*() {
        return new MPointerType(create_type<T>());
    }
};

template <typename T>
MPointerType *create_pointer_type() {
    return new MPointerType(create_type<T>());
}

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

//    llvm::Value *codegen_pool(JIT *jit, int num_elements);

    size_t _sizeof() { return 0; }

    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }

};

/*
 * MArrayType
 */

// MArrayType data type: MPointerType pointing to:
// MPrimType with type code: 3
// followed by
// MPrimType with type code: 5
// MPrimType with type code: 5
class MArrayType : public MStructType {
public:

    MArrayType(MType *user_type) : MStructType(mtype_marray) {
        MType *i = create_type<int>();
        MPointerType *user_ptr = new MPointerType(user_type);
        underlying_types.push_back(user_ptr);
        underlying_types.push_back(i);
        underlying_types.push_back(i);
        set_bits(user_ptr->get_bits() + i->get_bits() * 2);
        std::cerr << "MArray has this many bits: " << user_ptr->get_bits() + i->get_bits() * 2 << std::endl;
    }

    void dump();

//    // if the type is X, do X *x = (X*)malloc(sizeof(X) * num_elements);
//    void codegen_pool(JIT *jit, int num_elements, llvm::AllocaInst *malloc_dest) {
//        llvm::Type *marray_type = llvm::PointerType::get(codegen_type(), 0); // creates {i8*,i32,i32}*
//        size_t pool_size = sizeof(MArrayType) * num_elements;
//        llvm::Value *pool = CodegenUtils::codegen_c_malloc64_and_cast(jit, pool_size, llvm::);
//
////        llvm::Type *ptr_ptr_type = llvm::PointerType::get(llvm::PointerType::get(codegen(), 0), 0);
////        size_t malloc_size = sizeof(MArrayType*) * num_elements;
////        llvm::Value *malloced = CodegenUtils::codegen_c_malloc64_and_cast(jit, malloc_size, ptr_ptr_type);
////        jit->get_builder().CreateStore(malloced, malloc_dest);
//    }

    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }

};


template <typename T>
MArrayType *create_marraytype() {
    return new MArrayType(create_type<T *>());
};

/*
 * FileType
 */

class FileType : public MStructType {
public:

    FileType() : MStructType(mtype_file) {
        MType *i = create_type<int>(); // the length of the filepath
        MType *c = new MPointerType(create_type<char>()); // the filepath
        underlying_types.push_back(i);
        underlying_types.push_back(c);
    }

    void dump();

    size_t _sizeof() {
        return sizeof(File);
    }

    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }

//    void codegen_pool(JIT *jit, int num_elements) {
//        // creates {i32, i8*}*
//        llvm::Type *file_type = llvm::PointerType::get(codegen_type(), 0);
//        // allocate enough space for num_elements worth of FileType
//        size_t pool_size = sizeof(FileType) * num_elements;
//        // mallocs i8* and casts to {i32, i8*}*
//        llvm::Value *pool = CodegenUtils::codegen_c_malloc64_and_cast(jit, pool_size, file_type);
//    }

};

FileType *create_filetype();



/*
 * ElementType
 */

class ElementType : public MStructType {
private:

    MType *user_type;

public:

    ElementType(MType *user_type) : MStructType(mtype_element), user_type(user_type) {
        MType *i = create_type<long>();
        MType *user_ptr = new MPointerType(user_type);
        underlying_types.push_back(i); // tag value
        underlying_types.push_back(i); // data length
        underlying_types.push_back(user_ptr); // data array
    }

    void dump();

    MType *get_user_type() {
        return user_type;
    }

    // get the size of a full element
    size_t _sizeof() {
        switch (user_type->get_mtype_code()) {
            case mtype_bool:
                return sizeof(Element2<bool>);
            case mtype_char:
                return sizeof(Element2<char>);
            case mtype_short:
                return sizeof(Element2<short>);
            case mtype_int:
                return sizeof(Element2<int>);
            case mtype_long:
                return sizeof(Element2<long>);
            case mtype_float:
                return sizeof(Element2<float>);
            case mtype_double:
                return sizeof(Element2<double>);
            default:
                std::cerr << "bad user type for ElementType " << user_type->get_mtype_code() << std::endl;
                exit(8);
        }
    }

    size_t _sizeof_ptr() {
        switch (user_type->get_mtype_code()) {
            case mtype_bool:
                return sizeof(Element2<bool>*);
            case mtype_char:
                return sizeof(Element2<char>*);
            case mtype_short:
                return sizeof(Element2<short>*);
            case mtype_int:
                return sizeof(Element2<int>*);
            case mtype_long:
                return sizeof(Element2<long>*);
            case mtype_float:
                return sizeof(Element2<float>*);
            case mtype_double:
                return sizeof(Element2<double>*);
            default:
                std::cerr << "bad user type for ElementType" << user_type->get_mtype_code() << std::endl;
                exit(8);
        }
    }

    size_t _sizeof_T_type() {
        switch (user_type->get_mtype_code()) {
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
                std::cerr << "bad user type for ElementType" << user_type->get_mtype_code() << std::endl;
                exit(8);
        }
    }

    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function);
    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function);

    // preallocate a block of Element* objects that have a fixed size for data length
    // we will allocate space like this: {i32, i32, T*}**
    // T is the type passed into


//    void codegen_pool(JIT *jit, int num_elements) {
//        // creates {i32, i8*}*
//        llvm::Type *file_type = llvm::PointerType::get(codegen_type(), 0);
//        // allocate enough space for num_elements worth of FileType
//        size_t pool_size = sizeof(FileType) * num_elements;
//        // mallocs i8* and casts to {i32, i8*}*
//        llvm::Value *pool = CodegenUtils::codegen_c_malloc64_and_cast(jit, pool_size, file_type);
//    }

};

/*
 * ElementType
 */

//class ElementType : public MStructType {
//private:
//
//    mtype_code_t user_mtype_code;
//
//public:
//
//    ElementType(MType *user_type) : MStructType(mtype_element), user_mtype_code(user_type->get_mtype_code()) {
//        MType *i = create_type<int>(); // the length of the filepath
//        MType *c = new MPointerType(create_type<char>()); // the filepath
//        MType *user_ptr = new MPointerType(user_type);
//        underlying_types.push_back(i); // filepath length
//        underlying_types.push_back(i); // data length
//        underlying_types.push_back(c); // filepath
//        underlying_types.push_back(user_ptr); // data array
//    }
//
//    void dump();
//
//    size_t _sizeof() {
//        switch (user_mtype_code) {
//            case mtype_bool:
//                return sizeof(Element<bool>);
//            case mtype_char:
//                return sizeof(Element<char>);
//            case mtype_short:
//                return sizeof(Element<short>);
//            case mtype_int:
//                return sizeof(Element<int>);
//            case mtype_long:
//                return sizeof(Element<long>);
//            case mtype_float:
//                return sizeof(Element<float>);
//            case mtype_double:
//                return sizeof(Element<double>);
//            default:
//                std::cerr << "bad user type for ElementType";
//                exit(8);
//        }
//    }
//
////    void codegen_pool(JIT *jit, int num_elements) {
////        // creates {i32, i8*}*
////        llvm::Type *file_type = llvm::PointerType::get(codegen_type(), 0);
////        // allocate enough space for num_elements worth of FileType
////        size_t pool_size = sizeof(FileType) * num_elements;
////        // mallocs i8* and casts to {i32, i8*}*
////        llvm::Value *pool = CodegenUtils::codegen_c_malloc64_and_cast(jit, pool_size, file_type);
////    }
//
//};

/*
 * WrapperOutputType
 */

class WrapperOutputType : public MStructType {
public:

    WrapperOutputType(MType *user_type) : MStructType(mtype_wrapper_output) {
        MPointerType *user_ptr = new MPointerType(new MArrayType(new MPointerType(user_type)));
        underlying_types.push_back(user_ptr);
        set_bits(user_ptr->get_bits());
    }

    void dump();

//    llvm::Type* codegen();
    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }
};

/*
 * SegmentedElement
 */

class SegmentedElementType : public MStructType {
public:

    SegmentedElementType(MType *user_type) : MStructType(mtype_segmented_element) {
        MType *c = new MPointerType(new MArrayType(create_type<char>()));
        MType *user_ptr = new MPointerType(new MArrayType(user_type));
        MType *i = create_type<int>();
        underlying_types.push_back(c);
        underlying_types.push_back(user_ptr);
        underlying_types.push_back(i); // the offset
        set_bits(c->get_bits() + user_ptr->get_bits() + i->get_bits());
    }

    void dump();

//    llvm::Type* codegen();
    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }
};

/*
 * SegmentsType
 */

class SegmentsType : public MStructType {
public:

    SegmentsType(MType *user_type) : MStructType(mtype_segments) {
        MType *user_ptr = new MPointerType(new MArrayType(new MPointerType(new SegmentedElementType(user_type))));
//        MType *i = create_type<int>();
        underlying_types.push_back(user_ptr);
//        underlying_types.push_back(i);
        set_bits(user_ptr->get_bits());// + i->get_bits());
    }

    void dump();
    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

    }
//    llvm::Type* codegen();

};

/*
 * ComparisonElementType
 */

class ComparisonElementType : public MStructType {
public:

    ComparisonElementType(MType *user_type) : MStructType(mtype_comparison_element) {
        MType *c = new MPointerType(new MArrayType(create_type<char>()));
        MType *user_ptr = new MPointerType(new MArrayType(user_type));
        underlying_types.push_back(c);
        underlying_types.push_back(c);
        underlying_types.push_back(user_ptr);
        set_bits(c->get_bits() * 2 + user_ptr->get_bits());
    }

    void dump();

//    llvm::Type* codegen();
    llvm::AllocaInst * preallocate_block(JIT *jit, llvm::Value *num_elements, llvm::Value *fixed_data_length,
                                         llvm::Function *function) {

    }

    llvm::AllocaInst * preallocate_block(JIT *jit, int num_elements, int fixed_data_length, llvm::Function *function) {

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

template <>
struct mtype_of<FileType> {
    operator mtype_code_t() {
        return mtype_file;
    }
};

template <>
struct mtype_of<ElementType> {
    operator mtype_code_t() {
        return mtype_element;
    }
};

template <>
struct mtype_of<ComparisonElementType> {
    operator mtype_code_t() {
        return mtype_comparison_element;
    }
};

template <>
struct mtype_of<SegmentsType> {
    operator mtype_code_t() {
        return mtype_segments;
    }
};

template <>
struct mtype_of<SegmentedElementType> {
    operator mtype_code_t() {
        return mtype_segmented_element;
    }
};

// get how many counters are associated with a type so we know what to return from a stage
// For example, a FileType has a char* type in it, so from the stage we would want to return a single
// counter that contains all the chars we need to allocate space for all the char* types when we create input to the next stage
//template <typename T>
//struct get_num_size_fields {
//};
//
//template <>
//struct get_num_size_fields<FileType> {
//    operator int() {
//        return 1;
//    }
//};



#endif //MATCHIT_MTYPE_H
