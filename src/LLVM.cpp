//
// Created by Jessica Ray on 1/28/16.
//

#include <iostream>
#include "./LLVM.h"

namespace LLVM {

JIT *init() {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    return new JIT();
}

/*
 * Premade types
 */

llvm::Type *lvoid = llvm::Type::getVoidTy(llvm::getGlobalContext());

llvm::Type *lfloat = llvm::Type::getFloatTy(llvm::getGlobalContext());

llvm::PointerType *lfloatp = llvm::Type::getFloatPtrTy(llvm::getGlobalContext());

llvm::Type *ldouble = llvm::Type::getDoubleTy(llvm::getGlobalContext());

llvm::PointerType *ldoublep = llvm::Type::getDoublePtrTy(llvm::getGlobalContext());

llvm::IntegerType *lint1 = llvm::Type::getInt1Ty(llvm::getGlobalContext());

llvm::PointerType *lint1p = llvm::Type::getInt1PtrTy(llvm::getGlobalContext());

llvm::IntegerType *lint8 = llvm::Type::getInt8Ty(llvm::getGlobalContext());

llvm::PointerType *lint8p = llvm::Type::getInt8PtrTy(llvm::getGlobalContext());

llvm::IntegerType *lint16 = llvm::Type::getInt16Ty(llvm::getGlobalContext());

llvm::PointerType *lint16p = llvm::Type::getInt16PtrTy(llvm::getGlobalContext());

llvm::IntegerType *lint32 = llvm::Type::getInt32Ty(llvm::getGlobalContext());

llvm::PointerType *lint32p = llvm::Type::getInt32PtrTy(llvm::getGlobalContext());

llvm::IntegerType *lint64 = llvm::Type::getInt64Ty(llvm::getGlobalContext());

llvm::PointerType *lint64p = llvm::Type::getInt64PtrTy(llvm::getGlobalContext());

/*
 * MType
 */

llvm::Type *codegen_type(MVar *mvar) {
    return codegen_type(mvar->get_mtype());
}

llvm::Type *codegen_type(MType *mtype) {
    if (mtype->is_mscalar_type()) {
        return codegen_type((MScalarType *) mtype);
    } else if (mtype->is_mstruct_type()) {
        return codegen_type((MStructType *) mtype);
    } else if (mtype->is_marray_type()) {
        return codegen_type((MArrayType *) mtype);
    } else if (mtype->is_mmatrix_type()) {
        return codegen_type((MMatrixType *) mtype);
    } else if (mtype->is_mpointer_type()) {
        return codegen_type((MPointerType *) mtype);
    } else {
        assert(false && "I don't know that this type is");
    }
}

llvm::Type *codegen_type(MScalarType *mscalar_type) {
    switch (mscalar_type->get_mtype_code()) {
        case mtype_void:
            return lvoid;
        case mtype_bool:
            return lint1;
        case mtype_char:
            return lint8;
        case mtype_short:
            return lint16;
        case mtype_int:
            return lint32;
        case mtype_long:
            return lint64;
        case mtype_float:
            return lfloat;
        case mtype_double:
            return ldouble;
        default:
            assert(false && "non scalar type requested");
    }
}

llvm::Type *codegen_type(MPointerType *mpointer_type) {
    return llvm::PointerType::get(codegen_type(mpointer_type), 0);
}

llvm::Type *codegen_type(MStructType *mstruct_type) {
    std::vector<llvm::Type *> struct_field_types;
    for (size_t i = 0; i < mstruct_type->get_underlying_types().size(); i++) {
        struct_field_types.push_back(codegen_type(mstruct_type->get_underlying_types()[i]));
    }
    llvm::ArrayRef<llvm::Type *> struct_field_types_array(struct_field_types);
    return llvm::StructType::get(llvm::getGlobalContext(), struct_field_types_array);
}

llvm::Type *codegen_type(MArrayType *marray_type) {
    // TODO this just creates an LLVM pointer. Should I make an actual llvm array type and change the meaning of
    // array here?
    return llvm::PointerType::get(codegen_type(marray_type), 0);
}

llvm::Type *codegen_type(MMatrixType *mmatrix_type) {
    // TODO this just creates an LLVM pointer to a pointer. See comment in marray codegen above
    return llvm::PointerType::get(llvm::PointerType::get(codegen_type(mmatrix_type), 0), 0);
}

/*
 * Wrappers for constant values
 */

// only allows scalar types right now
llvm::Constant *as_constant(MType *mtype, void *val) {
    switch (mtype->get_mtype_code()) {
        case mtype_bool:
            if (*((bool*)val) == true) {
                return llvm::ConstantInt::getTrue(llvm::getGlobalContext());
            } else {
                return llvm::ConstantInt::getFalse(llvm::getGlobalContext());
            }
        case mtype_char:
            return llvm::ConstantInt::getSigned(llvm::Type::getInt8Ty(llvm::getGlobalContext()), *((char*)val));
        case mtype_short:
            return llvm::ConstantInt::getSigned(llvm::Type::getInt16Ty(llvm::getGlobalContext()), *((short*)val));
        case mtype_int:
            return llvm::ConstantInt::getSigned(llvm::Type::getInt32Ty(llvm::getGlobalContext()), *((int*)val));
        case mtype_long:
            return llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(llvm::getGlobalContext()), *((long*)val));
        case mtype_float:
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm::getGlobalContext()), *((float*)val));
        case mtype_double:
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(llvm::getGlobalContext()), *((double*)val));
        default:
            assert(false && "unhandled constant type");
    }
}

/*
 * LFunction
 */

MFunction *LFunction::get_mfunction() {
    return mfunction;
}

llvm::Function *LFunction::get_lfunction() {
    assert(lfunction && "llvm::Function not created yet! Need to run codegen().");
    return lfunction;
}

/*
 * LLVMCodeGenerator codegen
 */

void LLVMCodeGenerator::visit(MVar *mvar) {
    if (mvar->is_constant_val()) { // need to convert the data to an LLVM Constant
        llvm::Constant *constant = as_constant(mvar->get_mtype(), mvar->get_data<void>());
        mvar->set_data(constant); // no longer can directly access the c++ literal
        mvar->set_constant(false); // dirty hack to make sure we don't re-codegen this llvm::Constant as another llvm::Constant if it used multiple times
    } // else, just leave the mvar as is
}

void LLVMCodeGenerator::visit(MBlock *mblock) {
    // block for this iter has already been codegened by the previous iteration
    jit->get_builder().SetInsertPoint(mblock->get_data<llvm::BasicBlock>());
    for (size_t i = 0; i < mblock->get_exprs().size(); i++) {
        MExpr *expr = mblock->get_exprs()[i];
        expr->accept(this);
    }
}

void LLVMCodeGenerator::visit(MDirectBranch *mdbranch) {
    MBlock *branch_to = mdbranch->get_branch_block();
    llvm::BasicBlock *next_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), branch_to->get_name(),
                                                            current_function);
    branch_to->set_data(next_block);
    jit->get_builder().CreateBr(next_block);
}

void LLVMCodeGenerator::visit(MCondBranch *mcbranch) {
    MBlock *branch_to_true = mcbranch->get_branch_block_true();
    MBlock *branch_to_false = mcbranch->get_branch_block_false();
    llvm::BasicBlock *next_block_true = llvm::BasicBlock::Create(llvm::getGlobalContext(), branch_to_true->get_name(),
                                                                 current_function);
    llvm::BasicBlock *next_block_false = llvm::BasicBlock::Create(llvm::getGlobalContext(), branch_to_false->get_name(),
                                                                  current_function);
    branch_to_true->set_data(next_block_true);
    branch_to_false->set_data(next_block_false);
    jit->get_builder().CreateCondBr(mcbranch->get_actual_condition()->get_data<llvm::Value>(), next_block_true,
                                    next_block_false);
}

LFunction *LLVMCodeGenerator::visit(MFunction *mfunction) {
    // codegen prototype
    std::vector<llvm::Type *> arg_types;
    LFunction *lfunction = new LFunction(mfunction);
    for (size_t i = 0; i < mfunction->get_args().size(); i++) {
        arg_types.push_back(codegen_type(mfunction->get_args()[i]));
    }
    llvm::FunctionType *ft = llvm::FunctionType::get(codegen_type(mfunction->get_return_type()), arg_types,
                                                     mfunction->is_var_args());
    lfunction->set_lfunction(llvm::Function::Create(ft, llvm::Function::ExternalLinkage, mfunction->get_name(),
                                                    jit->get_module().get()));
    // give names to the function inputs
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry",
                                                       lfunction->get_lfunction()); // initial entry point
    current_function = lfunction->get_lfunction();
    jit->get_builder().SetInsertPoint(entry);
    int ctr = 0;
    for (llvm::Function::arg_iterator iter = lfunction->get_lfunction()->arg_begin();
         iter != lfunction->get_lfunction()->arg_end(); iter++) {
        iter->setName(mfunction->get_args()[ctr++]->get_name());
    }
    // codegen the body
    // but first, convert the MVars in the signature to llvm::Values b/c those will be used by some expr later down the line
    int arg_ctr = 0;
    for (llvm::Function::arg_iterator iter = lfunction->get_lfunction()->arg_begin();
         iter != lfunction->get_lfunction()->arg_end(); iter++) {
        MVar *marg = mfunction->get_args()[arg_ctr++];
        // have to store the args first
        llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(lint32);
        jit->get_builder().CreateStore(iter, alloc);
        marg->set_data(jit->get_builder().CreateLoad(alloc));
    }

    // directly branch to the first block
    if (mfunction->get_body().size() >= 1) {
        MBlock *block = mfunction->get_body()[0];
        llvm::BasicBlock *this_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), block->get_name(),
                                                                current_function);
        block->set_data(this_block);
        jit->get_builder().CreateBr(this_block);
    }

    for (size_t i = 0; i < mfunction->get_body().size(); i++) {
        MBlock *block = mfunction->get_body()[i];
        std::cerr << "block " << block->get_name() << std::endl;
        block->accept(this);
    }

    return lfunction;
}

void LLVMCodeGenerator::visit(MAdd *madd) {
    llvm::Value *val;
    // do any necessary codegen on parameters of this function
    madd->get_left()->accept(this);
    madd->get_right()->accept(this);
    if (madd->get_left()->get_mtype()->is_int_type()) {
        val = jit->get_builder().CreateAdd(madd->get_left()->get_data<llvm::Value>(),
                                           madd->get_right()->get_data<llvm::Value>());
    } else if (madd->get_left()->get_mtype()->is_float_type() || madd->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateFAdd(madd->get_left()->get_data<llvm::Value>(),
                                            madd->get_right()->get_data<llvm::Value>());
    } else {
        assert(false && "Type passed to add was not integer or floating point");
    }
    MVar *mvar = new MVar(madd->get_left()->get_mtype(), val, std::string("add_res")); // need explicitly make string, otherwise the boolean constructor is called
    madd->set_result(mvar);
}

void LLVMCodeGenerator::visit(MSub *msub) {
    llvm::Value *val;
    // do any necessary codegen on parameters of this function
    msub->get_left()->accept(this);
    msub->get_right()->accept(this);
    if (msub->get_left()->get_mtype()->is_int_type()) {
        val = jit->get_builder().CreateSub(msub->get_left()->get_data<llvm::Value>(),
                                           msub->get_right()->get_data<llvm::Value>());
    } else if (msub->get_left()->get_mtype()->is_float_type() || msub->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateSub(msub->get_left()->get_data<llvm::Value>(),
                                           msub->get_right()->get_data<llvm::Value>());
    } else {
        assert(false && "Type passed to sub was not integer or floating point");
    }
    MVar *mvar = new MVar(msub->get_left()->get_mtype(), val, std::string("sub_res"));
    msub->set_result(mvar);
}

void LLVMCodeGenerator::visit(MMul *mmul) {
    llvm::Value *val;
    // do any necessary codegen on parameters of this function
    mmul->get_left()->accept(this);
    mmul->get_right()->accept(this);
    if (mmul->get_left()->get_mtype()->is_int_type()) {
        val = jit->get_builder().CreateMul(mmul->get_left()->get_data<llvm::Value>(),
                                           mmul->get_right()->get_data<llvm::Value>());
    } else if (mmul->get_left()->get_mtype()->is_float_type() || mmul->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateFMul(mmul->get_left()->get_data<llvm::Value>(),
                                            mmul->get_right()->get_data<llvm::Value>());
    } else {
        assert(false && "Type passed to mul was not integer or floating point");
    }
    MVar *mvar = new MVar(mmul->get_left()->get_mtype(), val, std::string("mul_res"));
    mmul->set_result(mvar);
}

void LLVMCodeGenerator::visit(MDiv *mdiv) {
    llvm::Value *val;
    // do any necessary codegen on parameters of this function
    mdiv->get_left()->accept(this);
    mdiv->get_right()->accept(this);
    if (mdiv->get_left()->get_mtype()->is_signed_type()) {
        val = jit->get_builder().CreateSDiv(mdiv->get_left()->get_data<llvm::Value>(),
                                            mdiv->get_right()->get_data<llvm::Value>());
    } else if (mdiv->get_left()->get_mtype()->is_float_type() || mdiv->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateFDiv(mdiv->get_left()->get_data<llvm::Value>(),
                                            mdiv->get_right()->get_data<llvm::Value>());
    } else {
        assert(false && "Type passed to div was not (signed) integer or floating point");
    }
    MVar *mvar = new MVar(mdiv->get_left()->get_mtype(), val, std::string("div_res"));
    mdiv->set_result(mvar);
}

void LLVMCodeGenerator::visit(MSLT *mslt) {
    llvm::Value *val;
    // do any necessary codegen on parameters of this function
    mslt->get_left()->accept(this);
    mslt->get_right()->accept(this);
    val = jit->get_builder().CreateICmpSLT(mslt->get_left()->get_data<llvm::Value>(),
                                           mslt->get_right()->get_data<llvm::Value>());
    MVar *mvar = new MVar(mslt->get_left()->get_mtype(), val, std::string("slt_res"));
    mslt->set_result(mvar);
}

}