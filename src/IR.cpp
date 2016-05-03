//
// Created by Jessica Ray on 4/29/16.
//

#include "./IR.h"

/*
 * IR
 */

std::string &IR::get_name() {
    return name;
}

void IR::set_data(void *data) {
    this->data = data;
}

void IR::set_done() {
    done = true;
}

bool IR::is_done() {
    return done;
}

/*
 * MVar
 */

void MVar::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

MType *MVar::get_mtype() {
    return mtype;
}

bool MVar::is_constant_val() {
    return is_constant;
}

void MVar::set_constant(bool is_constant) {
    this->is_constant = is_constant;
}

/*
 * MFunction
 */

std::vector<MVar *> MFunction::get_args() {
    return args;
}

std::vector<MBlock *> MFunction::get_body() {
    return body;
}

MType *MFunction::get_return_type() {
    return return_type;
}

bool MFunction::is_var_args() {
    return var_args;
}

void MFunction::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

void MFunction::insert(MBlock *block) {
    body.push_back(block);
}

bool MFunction::is_prototype_only() {
    return prototype_only;
}

void MFunction::add_args(std::vector<MVar *> args) {
    this->args = args;
}

/*
 * MFunctionCall
 */

MFunction *MFunctionCall::get_mfunction() {
    return mfunction;
}

void MFunctionCall::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

/*
 * MRetVal
 */

void MRetVal::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

MVar *MRetVal::get_ret_val() {
    return ret_val;
}

/*
 * MBlock
 */

void MBlock::insert(MExpr *mexpr) {
    exprs.push_back(mexpr);
}

std::vector<MExpr *> MBlock::get_exprs() {
    return exprs;
}

void MBlock::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

/*
 * MDirectBranch
 */

void MDirectBranch::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

MBlock *MDirectBranch::get_branch_block() {
    return branch_to;
}

/*
 * MCondBranch
 */

void MCondBranch::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

MBlock *MCondBranch::get_branch_block_true() {
    return branch_to_true;
}

MBlock *MCondBranch::get_branch_block_false() {
    return branch_to_false;
}

MVar *MCondBranch::get_actual_condition() {
    return condition[0];
}

MVar **MCondBranch::get_condition() {
    return condition;
}

/*
 * MBinaryOp
 */

MVar *MBinaryOp::get_left() {
    return left[0];
}

MVar *MBinaryOp::get_right() {
    return right[0];
}

MVar *MBinaryOp::get_actual_result() {
    return result[0];
}

MVar **MBinaryOp::get_result() {
    return result;
}

void MBinaryOp::set_result(MVar *res) {
    result[0] = res;
}

/*
 * MAdd
 */

void MAdd::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

/*
 * MSub
 */

void MSub::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

/*
 * MMul
 */

void MMul::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

/*
 * MDiv
 */

void MDiv::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}

/*
 * MLT
 */

void MSLT::accept(MIRVisitor *visitor) {
    visitor->visit(this);
}