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

// an MVar can hold many kinds of data. When we need a loadinst (i.e. the actual data), check if the data is an AllocaInst,
// and then load if necessary
llvm::Value *LLVMCodeGenerator::load_mvar(MVar *mvar) {
    llvm::Value *cur_val = mvar->get_data<llvm::Value>();
    bool is_alloca = llvm::AllocaInst::classof(cur_val);
    if (is_alloca) {
        return jit->get_builder().CreateLoad(cur_val);
    } else {
        return cur_val;
    }
}

void LLVMCodeGenerator::visit(MVar *mvar) {
    if (mvar->is_constant_val() && !mvar->is_done()) { // need to convert the data to an LLVM Constant
        llvm::Constant *constant = as_constant(mvar->get_mtype(), mvar->get_data<void>());
        mvar->set_data(constant); // no longer can directly access the c++ literal
//        mvar->set_constant(false); // hack to make sure we don't re-codegen this llvm::Constant as another llvm::Constant if it used multiple times
        mvar->set_done();
    }
    // else, just leave the mvar as is
}

void LLVMCodeGenerator::visit(MFunctionCall *mfunction_call) {
    std::vector<llvm::Value *> call_args;
    std::vector<MVar **> margs = mfunction_call->get_mfunction()->get_args();
    for (size_t i = 0; i < margs.size(); i++) {
        margs[i][0]->accept(this);
        call_args.push_back(load_mvar(margs[i][0]));
    }
    mfunction_call->get_mfunction()->accept(this);
    jit->get_builder().CreateCall(mfunction_call->get_mfunction()->get_data<llvm::Function>(), call_args);
}

void LLVMCodeGenerator::visit(MFor *mfor) {
    // codegen the MVars if needed
    mfor->get_start()->accept(this);
    mfor->get_loop_bound()->accept(this);
    mfor->get_loop_index()->accept(this);
    mfor->get_step_size()->accept(this);

    llvm::BasicBlock *body_block = llvm::BasicBlock::Create(llvm::getGlobalContext(),
                                                            mfor->get_body_block()->get_name(), current_function);
    mfor->get_body_block()->set_data(body_block);

    MBlock *condition_mblock = new MBlock("loop_condition");
    llvm::BasicBlock *condition_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), condition_mblock->get_name(),
                                                                 current_function);
    condition_mblock->set_data(condition_block);
//    mfor->set_condition_block(condition_mblock);

    MBlock *increment_mblock = new MBlock("loop_increment");
    llvm::BasicBlock *increment_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), increment_mblock->get_name(),
                                                                 current_function);
    increment_mblock->set_data(increment_block);
//    mfor->set_increment_block(increment_mblock);

    llvm::BasicBlock *end_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), mfor->get_end_block()->get_name(),
                                                           current_function);
    mfor->get_end_block()->set_data(end_block);

    // make the loop index
    // loop index is the current iteration of the loop, starting at "start"
    llvm::Value *start_val = load_mvar(mfor->get_start());
    llvm::AllocaInst *loop_index = jit->get_builder().CreateAlloca(start_val->getType(), nullptr, "loop_index");
    jit->get_builder().CreateStore(start_val, loop_index);
    mfor->get_loop_index()->set_data(loop_index); // we now have access to the loop iteration
    jit->get_builder().CreateBr(condition_block);

    // make the condition check
    jit->get_builder().SetInsertPoint(condition_block);
    llvm::Value *cond = jit->get_builder().CreateICmpSLT(load_mvar(mfor->get_loop_index()),
                                                         load_mvar(mfor->get_loop_bound()));
    jit->get_builder().CreateCondBr(cond, mfor->get_body_block()->get_data<llvm::BasicBlock>(),
                                    end_block);

    // make the increment
    jit->get_builder().SetInsertPoint(increment_block);
    llvm::Value *cur_loop_index = jit->get_builder().CreateLoad(loop_index);
    llvm::Value *incremented = jit->get_builder().CreateAdd(cur_loop_index, load_mvar(mfor->get_step_size()));
    jit->get_builder().CreateStore(incremented, loop_index);
    jit->get_builder().CreateBr(condition_block);

    // make the body
    mfor->get_body_block()->accept(this);
    jit->get_builder().CreateBr(increment_block);

    // make the end block (empty until code following the for loop is generated)
//    jit->get_builder().SetInsertPoint(end_block);

}

void LLVMCodeGenerator::visit(MIfThenElse *mif_then_else) {
    MBlock *if_mblock = mif_then_else->get_if_mblock();
    MBlock *else_mblock = mif_then_else->get_else_mblock();
    MBlock *after_if_mblock = mif_then_else->get_after_if_mblock();
    MBlock *after_else_mblock = mif_then_else->get_after_else_mblock();
    MVar *condition = mif_then_else->get_actual_condition();
    // structure is:
    // conditional branch (condition, if_block, else_block)
    // if block: ...
    // else block: ...

    // codegen the MVar if needed
    visit(condition);

    // create the LLVM side of things
    llvm::BasicBlock *if_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), if_mblock->get_name(),
                                                            current_function);
    llvm::BasicBlock *else_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), else_mblock->get_name(),
                                                             current_function);
    if_mblock->set_data(if_block);
    else_mblock->set_data(else_block);

    // figure out what is happening AFTER the if/then/else
    llvm::BasicBlock *after_block;
    if (after_if_mblock == after_else_mblock && after_if_mblock != nullptr) { // joiner
        after_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), after_if_mblock->get_name(),
                                               current_function);
        after_if_mblock->set_data(after_block);
        after_else_mblock->set_data(after_block);
    } else if (after_if_mblock != nullptr) {
        after_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), after_if_mblock->get_name(),
                                               current_function);
        after_if_mblock->set_data(after_block);
    } else if (after_else_mblock != nullptr) {
        after_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), after_else_mblock->get_name(),
                                               current_function);
        after_else_mblock->set_data(after_block);
    }

    jit->get_builder().CreateCondBr(condition->get_data<llvm::Value>(), if_block, else_block);
    // codegen the if block, then the else block
    // insert a branch to an ending block that is what will run after the if/else stuff is done
    visit(if_mblock);
    if (after_if_mblock != nullptr) {
        jit->get_builder().CreateBr(after_block);
    }
    visit(else_mblock);
    if (after_else_mblock != nullptr) {
        jit->get_builder().CreateBr(after_block);
    }
}

void LLVMCodeGenerator::visit(MBlock *mblock) {
    // block for this iter has already been codegened by the previous iteration
    jit->get_builder().SetInsertPoint(mblock->get_data<llvm::BasicBlock>());
    for (size_t i = 0; i < mblock->get_exprs().size(); i++) {
        MExpr *expr = mblock->get_exprs()[i];
        expr->accept(this);
    }
}

void LLVMCodeGenerator::visit(MRetVal *mret_val) {
    if (mret_val->get_ret_val()) {
        mret_val->get_ret_val()->accept(this);
        jit->get_builder().CreateRet(load_mvar(mret_val->get_ret_val()));
    } else {
        jit->get_builder().CreateRetVoid();
    }
}

void LLVMCodeGenerator::visit(MFunction *mfunction) {
    // codegen prototype
    LFunction *lfunction = new LFunction(mfunction);
    if (!mfunction->is_done()) {
        std::vector<llvm::Type *> arg_types;
        for (size_t i = 0; i < mfunction->get_args().size(); i++) {
            arg_types.push_back(codegen_type(mfunction->get_args()[i][0]));
        }
        llvm::FunctionType *ft = llvm::FunctionType::get(codegen_type(mfunction->get_return_type()), arg_types,
                                                         mfunction->is_var_args());
        lfunction->set_lfunction(llvm::Function::Create(ft, llvm::Function::ExternalLinkage, mfunction->get_name(),
                                                        jit->get_module().get()));
        mfunction->set_data(lfunction->get_lfunction());
        jit->get_module()->getOrInsertFunction(mfunction->get_name(), ft);
        mfunction->set_done();
    }
    if (!mfunction->is_prototype_only()) { // if it's a prototype, then it's something like an extern function call so we don't codegen the body
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry",
                                                           lfunction->get_lfunction()); // initial entry point
        current_function = lfunction->get_lfunction();
        jit->get_builder().SetInsertPoint(entry);
        // give names to the function inputs
        int ctr = 0;
        for (llvm::Function::arg_iterator iter = lfunction->get_lfunction()->arg_begin();
             iter != lfunction->get_lfunction()->arg_end(); iter++) {
            iter->setName(mfunction->get_args()[ctr++][0]->get_name());
        }
        // codegen the body
        // but first, convert the MVars in the signature to llvm::Values b/c those will be used by some expr later down the line
        int arg_ctr = 0;
        for (llvm::Function::arg_iterator iter = lfunction->get_lfunction()->arg_begin();
             iter != lfunction->get_lfunction()->arg_end(); iter++) {
            MVar *marg = mfunction->get_args()[arg_ctr++][0];
            // have to store the args first
            llvm::AllocaInst *alloc = jit->get_builder().CreateAlloca(codegen_type(marg->get_mtype()));
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
            block->accept(this);
        }
    }

//    return lfunction;
}

void LLVMCodeGenerator::visit(MAdd *madd) {
    llvm::Value *val;
    // do any necessary codegen on parameters of this function
    madd->get_left()->accept(this);
    madd->get_right()->accept(this);
    if (madd->get_left()->get_mtype()->is_int_type()) {
        val = jit->get_builder().CreateAdd(load_mvar(madd->get_left()),
                                           load_mvar(madd->get_right()));
    } else if (madd->get_left()->get_mtype()->is_float_type() || madd->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateFAdd(load_mvar(madd->get_left()),
                                            load_mvar(madd->get_right()));
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
        val = jit->get_builder().CreateSub(load_mvar(msub->get_left()),
                                           load_mvar(msub->get_right()));
    } else if (msub->get_left()->get_mtype()->is_float_type() || msub->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateSub(load_mvar(msub->get_left()),
                                           load_mvar(msub->get_right()));
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
        val = jit->get_builder().CreateMul(load_mvar(mmul->get_left()),
                                           load_mvar(mmul->get_right()));
    } else if (mmul->get_left()->get_mtype()->is_float_type() || mmul->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateFMul(load_mvar(mmul->get_left()),
                                            load_mvar(mmul->get_right()));
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
        val = jit->get_builder().CreateSDiv(load_mvar(mdiv->get_left()),
                                            load_mvar(mdiv->get_right()));
    } else if (mdiv->get_left()->get_mtype()->is_float_type() || mdiv->get_left()->get_mtype()->is_double_type()) {
        val = jit->get_builder().CreateFDiv(load_mvar(mdiv->get_left()),
                                            load_mvar(mdiv->get_right()));
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
    val = jit->get_builder().CreateICmpSLT(load_mvar(mslt->get_left()),
                                           load_mvar(mslt->get_right()));
    MVar *mvar = new MVar(mslt->get_left()->get_mtype(), val, std::string("slt_res"));
    mslt->set_result(mvar);
}

}