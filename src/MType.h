//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include "llvm/IR/Type.h"

#ifndef MATCHIT_MTYPE_H
#define MATCHIT_MTYPE_H

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
    mtype_stage // 17 -- the return struct that wraps a user return type
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
    virtual llvm::Type *codegen() = 0;

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

    llvm::Type *codegen();

    void dump();

};

class MPointerType : public MType {
public:

    MPointerType() {}

    MPointerType(MType *pointer_type) : MType(mtype_ptr, 64) {
        underlying_types.push_back(pointer_type);
    } // TODO 64 is platform specific

    ~MPointerType() {}

    llvm::Type *codegen();

    void dump();

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

/*
 * MArrayType
 */

// MArrayType data type: MPointerType pointing to:
// MPrimType with type code: 3
// followed by
// MPrimType with type code: 5
// MPrimType with type code: 5
class MArrayType : public MType {
public:

    MArrayType(MType *user_type) : MType(mtype_marray, 0) {
        MType *i = create_type<int>();
        underlying_types.push_back(new MPointerType(user_type));
        underlying_types.push_back(i);
        underlying_types.push_back(i);
        set_bits(user_type->get_bits() + i->get_bits() * 2);
    }

    void dump();

    llvm::Type *codegen();
};

template <typename T>
MArrayType *create_marraytype() {
    return new MArrayType(create_type<T *>());
};

/*
 * FileType
 */

//FileType has field type MPointerType pointing to:
//        MArrayType data type: MPointerType pointing to:
//        MPrimType with type code: 3
//followed by
//MPrimType with type code: 5
//MPrimType with type code: 5
class FileType : public MType {
public:

    FileType() : MType() {
        MType *c = new MPointerType(new MArrayType(create_type<char>()));
        underlying_types.push_back(c);
    }

    void dump();

    llvm::Type *codegen();

};

FileType *create_filetype();

/*
 * ElementType
 */

//ElementType has field types MPointerType pointing to:
//        MArrayType data type: MPointerType pointing to:
//        MPrimType with type code: 3
//followed by
//MPrimType with type code: 5
//MPrimType with type code: 5
//and MPointerType pointing to:
//        MArrayType data type: MPointerType pointing to:
//        MPrimType with type code: 3
//followed by
//MPrimType with type code: 5
//MPrimType with type code: 5
class ElementType : public MType {
public:

    ElementType(MType *user_type) : MType() {
        MType *c = new MPointerType(new MArrayType(create_type<char>()));
        underlying_types.push_back(c);
        underlying_types.push_back(new MPointerType(new MArrayType(user_type)));
    }

    void dump();

    llvm::Type *codegen();

};

/*
 * WrapperOutputType
 */

// Element with type float
//WrapperOutputType has field types MPointerType pointing to:
//        MArrayType data type: MPointerType pointing to:
//        MPointerType pointing to:
//ElementType has field types MPointerType pointing to:
//        MArrayType data type: MPointerType pointing to:
//        MPrimType with type code: 3
//followed by
//MPrimType with type code: 5
//MPrimType with type code: 5
//and MPointerType pointing to:
//        MArrayType data type: MPointerType pointing to:
//        MPrimType with type code: 7
//followed by
//MPrimType with type code: 5
//MPrimType with type code: 5
//followed by
//MPrimType with type code: 5
//MPrimType with type code: 5
class WrapperOutputType : public MType {
public:

    WrapperOutputType(MType *user_type) : MType() {
        underlying_types.push_back(new MPointerType(new MArrayType(new MPointerType(user_type))));
    }

    void dump();

    llvm::Type *codegen();

};

//template <template<class> class T, class S>
//struct create_type<T<S>> {};
//
//template <template<class> class T, class S>
//struct create_type<ElementType> {
//
//};
//template <typename T>
//struct create_type<ElementType> {
//    operator ElementType*() {
//
//    }
//};
//
//template <typename T>
//ElementType *create_elementtype() {
//    return new ElementType(create_marraytype<T>());
//}

//template <typename T, typename S>
//WrapperOutputType *create_wrapperoutputtype() {
//    return new WrapperOutputType(create_marraytype<T *>());
//}

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



//template <>
//struct create_type<FileType> {
//    operator MType*() {
//        std::cerr << "Creating type file" << std::endl;
//        return new FileType();
//    }
//};

//template <typename T>
//struct create_type<ElementType<T>> {
//    operator MType*() {
//        return new ElementType<create_type<T>()>();
//    }
//};
//
//template <typename T>
//struct create_type<ComparisonElementType<T>> {
//operator MType*() {
//    return new ComparisonElementType<create_type<T>()>();
//}
//};
//
//template <typename T>
//struct create_type<SegmentedElementType<T>> {
//operator MType*() {
//    return new SegmentedElementType<create_type<T>()>();
//}
//};
//
//template <typename T>
//struct create_type<SegmentsType<T>> {
//operator MType*() {
//    return new SegmentsType<create_type<T>()>();
//}
//};
//
//template <typename T>
//struct create_type<WrapperOutputType<T>> {
//operator MType*() {
//    return new WrapperOutputType<create_type<T>()>();
//}
//};








//template <typename T>
//struct create_type<MArrayType> {
//    operator MArrayType*() {
//        return new MArrayType(create_type<T>());
//    }
//};



//template <typename T>
//class WrapperOutputType : public MType {
//public:
//
//    WrapperOutputType() : MType() {
//        MType *t = create_type<MArrayType<T> **>();
//        MType *i = create_type<unsigned int>();
//        underlying_types.push_back(t);
//        underlying_types.push_back(i);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//class FileType : public MType {
//public:
//
//    FileType() : MType() {
//        MType *c = create_type<MArrayType<char> *>();
//        underlying_types.push_back(c);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class ElementType : public MType {
//public:
//
//    ElementType() : MType() {
//        MType *c = create_type<MArrayType<char> *>();
//        MType *t = create_type<MArrayType<T> *>();
//        underlying_types.push_back(c);
//        underlying_types.push_back(t);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class SegmentedElementType : public MType {
//public:
//
//    SegmentedElementType() : MType() {
//        MType *c = create_type<MArrayType<char> *>();
//        MType *t = create_type<MArrayType<T> *>();
//        MType *i = create_type<unsigned int>();
//        underlying_types.push_back(c);
//        underlying_types.push_back(t);
//        underlying_types.push_back(i);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class SegmentsType : public MType {
//public:
//
//    SegmentsType() {
//        MType *s = create_type<MArrayType<SegmentedElementType<T> *> *>();
//        underlying_types.push_back(s);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class ComparisonElementType : public MType {
//public:
//
//    ComparisonElementType() {
//        MType *c = create_type<MArrayType<char> *>();
//        MType *t = create_type<MArrayType<T> *>();
//        underlying_types.push_back(c);
//        underlying_types.push_back(c);
//        underlying_types.push_back(t);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};


/*
 * Create MTypes from C types
 */




/*
 * Data types available for the user in their stages
 */

//template <typename T>
//class MArrayType : public MType {
//public:
//
//    MArrayType() : MType() {
//        MType *t = create_type<T *>();
//        MType *i = create_type<int>();
//        underlying_types.push_back(t);
//        underlying_types.push_back(i);
//        underlying_types.push_back(i);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//
//
////class BaseElementType : public MType {
////public:
////
////    BaseElementType() { }
////
////    void dump() {
////
////    }
////
////    llvm::Type *codegen() {
////
////    }
////
////};
//
////MType *c = create_type<MArrayType<char> *>();
////underlying_types.push_back(c);
//
//template <typename T>
//class WrapperOutputType : public MType {
//public:
//
//    WrapperOutputType() : MType() {
//        MType *t = create_type<MArrayType<T> **>();
//        MType *i = create_type<unsigned int>();
//        underlying_types.push_back(t);
//        underlying_types.push_back(i);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//class FileType : public MType {
//public:
//
//    FileType() : MType() {
//        MType *c = create_type<MArrayType<char> *>();
//        underlying_types.push_back(c);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class ElementType : public MType {
//public:
//
//    ElementType() : MType() {
//        MType *c = create_type<MArrayType<char> *>();
//        MType *t = create_type<MArrayType<T> *>();
//        underlying_types.push_back(c);
//        underlying_types.push_back(t);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class SegmentedElementType : public MType {
//public:
//
//    SegmentedElementType() : MType() {
//        MType *c = create_type<MArrayType<char> *>();
//        MType *t = create_type<MArrayType<T> *>();
//        MType *i = create_type<unsigned int>();
//        underlying_types.push_back(c);
//        underlying_types.push_back(t);
//        underlying_types.push_back(i);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class SegmentsType : public MType {
//public:
//
//    SegmentsType() {
//        MType *s = create_type<MArrayType<SegmentedElementType<T> *> *>();
//        underlying_types.push_back(s);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//template <typename T>
//class ComparisonElementType : public MType {
//public:
//
//    ComparisonElementType() {
//        MType *c = create_type<MArrayType<char> *>();
//        MType *t = create_type<MArrayType<T> *>();
//        underlying_types.push_back(c);
//        underlying_types.push_back(c);
//        underlying_types.push_back(t);
//    }
//
//    void dump() { }
//
//    llvm::Type *codegen() { }
//
//};
//
//
///*
// * Create MTypes from C types
// */
//
//
//template <>
//struct create_type<FileType> {
//    operator MType*() {
//        std::cerr << "Creating type file" << std::endl;
//        return new FileType();
//    }
//};
//
//template <typename T>
//struct create_type<ElementType<T>> {
//    operator MType*() {
//        return new ElementType<create_type<T>()>();
//    }
//};
//
//template <typename T>
//struct create_type<ComparisonElementType<T>> {
//    operator MType*() {
//        return new ComparisonElementType<create_type<T>()>();
//    }
//};
//
//template <typename T>
//struct create_type<SegmentedElementType<T>> {
//    operator MType*() {
//        return new SegmentedElementType<create_type<T>()>();
//    }
//};
//
//template <typename T>
//struct create_type<SegmentsType<T>> {
//    operator MType*() {
//        return new SegmentsType<create_type<T>()>();
//    }
//};
//
//template <typename T>
//struct create_type<WrapperOutputType<T>> {
//    operator MType*() {
//        return new WrapperOutputType<create_type<T>()>();
//    }
//};

// TODO for now, only pointers in and out of stages are allowed. I think.
//template <typename T>
//struct create_type {
//    operator MType*() {
//        std::cerr << "MType.h: Shouldn't be in here (create_type for typename T)!" << std::endl;
//        exit(19);
//
//    }
//};

// TODO try to get rid of these and the mtype_struct in general

//MStructType *create_struct_type(std::vector<MType *> field_types) {
//    return new MStructType(mtype_struct, field_types);
//}
//
//MStructType *create_struct_type(mtype_code_t struct_type, std::vector<MType *> field_types) {
//    return new MStructType(struct_type, field_types);
//}
//
//MPointerType *create_struct_reference_type(std::vector<MType *> field_types) {
//    return new MPointerType(create_struct_type(field_types));
//}
//
//MPointerType *create_struct_reference_type(mtype_code_t struct_type, std::vector<MType *> field_types) {
//    return new MPointerType(create_struct_type(struct_type, field_types));
//}


/*
 * Return the mtype_code_t value for a given C type or MType struct
 */



//template <>
//struct mtype_of<File> {
//    operator mtype_code_t() {
//        return mtype_file;
//    }
//};
//
//template <typename T>
//struct mtype_of<Element<T> > {
//    operator mtype_code_t() {
//        return mtype_element;
//    }
//};
//
//template <typename T>
//struct mtype_of<ComparisonElement<T> > {
//    operator mtype_code_t() {
//        return mtype_comparison_element;
//    }
//};
//
//template <typename T>
//struct mtype_of<Segments<T> > {
//    operator mtype_code_t() {
//        return mtype_segments;
//    }
//};
//
//template <typename T>
//struct mtype_of<SegmentedElement<T> > {
//    operator mtype_code_t() {
//        return mtype_segmented_element;
//    }
//};



#endif //MATCHIT_MTYPE_H
