//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_LLVM_H
#define MATCHIT_LLVM_H

#include "./JIT.h"
#include "./IR.h"

namespace LLVM {

JIT *init();

/*
 * Some premade LLVM types
 */

extern llvm::Type *lvoid;

extern llvm::Type *lfloat;

extern llvm::PointerType *lfloatp;

extern llvm::Type *ldouble;

extern llvm::PointerType *ldoublep;

extern llvm::IntegerType *lint1;

extern llvm::PointerType *lint1p;

extern llvm::IntegerType *lint8;

extern llvm::PointerType *lint8p;

extern llvm::IntegerType *lint16;

extern llvm::PointerType *lint16p;

extern llvm::IntegerType *lint32;

extern llvm::PointerType *lint32p;

extern llvm::IntegerType *lint64;

extern llvm::PointerType *lint64p;

/*
 * LFunction
 */

class LFunction {
private:

    MFunction *mfunction;
    llvm::Function *lfunction;

public:

    LFunction(MFunction *mfunction) : mfunction(mfunction) { }

    MFunction *get_mfunction();

    llvm::Function *get_lfunction();

    void codegen_prototype(JIT *jit);

    void codegen_body(JIT *jit);

    void set_lfunction(llvm::Function *lfunction) {
        this->lfunction = lfunction;
    }

};

/*
 * LFunctionCall
 */

//class LFunctionCall {
//private:
//
//    MFunctionCall *mfunction_call;
//
//};

/*
 * stuffff
 */

llvm::Type *codegen_type(MVar *mvar);

llvm::Type *codegen_type(MType *mtype);

llvm::Type *codegen_type(MScalarType *mscalar_type);

llvm::Type *codegen_type(MPointerType *mpointer_type);

llvm::Type *codegen_type(MStructType *mstruct_type);

llvm::Type *codegen_type(MArrayType *marray_type);

llvm::Type *codegen_type(MMatrixType *mmatrix_type);

class LLVMCodeGenerator : public MIRVisitor { //<llvm::Value *> {
private:

    JIT *jit;
    llvm::Function *current_function;

public:

    LLVMCodeGenerator(JIT *jit) : jit(jit) { }

    void visit(MFunction *mfunction);
    void visit(MVar *mvar);
    void visit(MFunctionCall *mfunction_call);
    void visit(MRetVal *mret_val);
    void visit(MBlock *mblock);
    void visit(MDirectBranch *mdbranch);
    void visit(MCondBranch *mcbranch);
    void visit(MAdd *madd); // from comment above, if this was template here I would have to define everything in this file
    void visit(MSub *msub);
    void visit(MMul *mmul);
    void visit(MDiv *mdiv);
    void visit(MSLT *mslt);

};


}




#endif //MATCHIT_LLVM_H
