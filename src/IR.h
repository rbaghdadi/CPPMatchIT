//
// Created by Jessica Ray on 4/29/16.
//

#ifndef MATCHIT_IR_H
#define MATCHIT_IR_H

#include "./MType.h"

class MVar;
class MFunction;
class MFunctionCall;
class MRetVal;
class MBlock;
class MBranch;
class MDirectBranch;
class MCondBranch;
class MAdd;
class MSub;
class MMul;
class MDiv;
class MSLT;

/*
 * Visitor
 */

class MIRVisitor {
public:

    MIRVisitor() { }

    virtual ~MIRVisitor() { }

    virtual void visit(MVar *mvar) = 0;
    virtual void visit(MFunction *mfunction) = 0;
    virtual void visit(MFunctionCall *mfunction_call) = 0;
    virtual void visit(MRetVal *mret_val) = 0;
    virtual void visit(MBlock *mblock) = 0;
    virtual void visit(MDirectBranch *mdbranch) = 0;
    virtual void visit(MCondBranch *mcbranch) = 0;
    virtual void visit(MAdd *madd) = 0;
    virtual void visit(MSub *msub) = 0;
    virtual void visit(MMul *mmul) = 0;
    virtual void visit(MDiv *mdiv) = 0;
    virtual void visit(MSLT *mslt) = 0;

};

/*
 * IR
 */

class IR {
private:

    std::string name;
    void *data; // use to hold stuff to pass around nodes (if needed)
    bool done;

public:

    IR() : name(""), done(false) { }

    IR(void *data) : data(data), done(false) { }

    IR(std::string name) : name(name), done(false) { }

    IR(std::string name, void *data) : name(name), data(data), done(false) { }

    virtual ~IR() { }

    std::string &get_name();

    virtual void accept(MIRVisitor *visitor) = 0;

    template <typename T>
    T *get_data() {
        return (T*)data;
    }

    void set_data(void *data);

    void set_done();

    bool is_done();

};

/*
 * MVar
 */

// like a new value, a parameter, something like that
class MVar : public IR {
private:

    MType *mtype;
    bool is_constant;

public:

    MVar(MType *mtype) : mtype(mtype), is_constant(false) { }

    MVar(MType *mtype, void *data) : IR(data), mtype(mtype), is_constant(false) { }

    MVar(MType *mtype, std::string name) : IR(name), mtype(mtype), is_constant(false) { }

    MVar(MType *mtype, void *data, std::string name) : IR(name, data), mtype(mtype), is_constant(false) { }

    MVar(MType *mtype, void *data, bool is_constant) : IR(data), mtype(mtype), is_constant(is_constant) { }

    MVar(MType *mtype, void *data, std::string name, bool is_constant) : IR(name, data), mtype(mtype),
                                                                         is_constant(is_constant) { }

    virtual ~MVar() { }

    MType *get_mtype();

    void accept(MIRVisitor *visitor);

    bool is_constant_val();

    void set_constant(bool is_constant);
};

// just some type of expression
class MExpr : public IR {

public:

    MExpr() { }

    MExpr(std::string name) : IR(name) { }

    virtual ~MExpr() { }

    virtual void accept(MIRVisitor *visitor) = 0;

};

/*
 * MFunction
 */

class MFunction : public IR {
private:

    std::vector<MVar *> args; // TODO this needs to be std::vector<MVar **> so that I can pass in not-yet-created args
    MType *return_type;
    bool var_args;
    std::vector<MBlock *> body;
    bool prototype_only;

public:

    MFunction(std::string name, MType *return_type) : IR(name), return_type(return_type), prototype_only(false) { }

    MFunction(std::string name, MType *return_type, bool prototype_only) : IR(name), return_type(return_type),
                                                                           prototype_only(prototype_only) { }

    MFunction(std::string name, std::vector<MVar *> args, MType *return_type) : IR(name), args(args),
                                                                                return_type(return_type),
                                                                                var_args(false),
                                                                                prototype_only(false) { }

    MFunction(std::string name, std::vector<MVar *> args, MType *return_type, bool prototype_only) :
            IR(name), args(args), return_type(return_type), var_args(false), prototype_only(prototype_only) { }

//    MFunction(std::string name, std::vector<MVar *> args, MType *return_type, bool var_args) : IR(name), args(args),
//                                                                                               return_type(return_type),
//                                                                                               var_args(var_args),
//                                                                                               prototype_only(false) { }

    virtual ~MFunction() { }

    std::vector<MVar *> get_args();

    MType *get_return_type();

    bool is_var_args();

    void insert(MBlock *block);

    std::vector<MBlock *> get_body();

    void accept(MIRVisitor *visitor);

    bool is_prototype_only();

    void add_args(std::vector<MVar *> args);

};

/*
 * MFor
 */

//class MFor : public IR {
//private:
//
//    MVar start;
//    MVar end;
//    MVar step_size;
//    MVar index;
//
//public:
//
//};
//

/*
 * MFunctionCall
 */

class MFunctionCall : public MExpr {
private:

    MFunction *mfunction;
    std::vector<MVar *> args;

public:

    MFunctionCall(MFunction *mfunction) : mfunction(mfunction) { }

    MFunctionCall(MFunction *mfunction, std::string name) : MExpr(name), mfunction(mfunction) { }

    virtual ~MFunctionCall() { }

    MFunction *get_mfunction();

    void accept(MIRVisitor *visitor);

};

/*
 * MRetVal
 */

class MRetVal : public MExpr {
private:

    MVar *ret_val;

public:

    MRetVal() : ret_val(nullptr) { }

    MRetVal(MVar *ret_val) : ret_val(ret_val) { }

    virtual ~MRetVal() { }

    void accept(MIRVisitor *visitor);

    MVar *get_ret_val();

};

/*
 * MBlock
 */

class MBlock : public IR {
private:

    std::vector<MExpr *> exprs;

public:

    MBlock(std::string name) : IR(name) { }

    virtual ~MBlock() { }

    void accept(MIRVisitor *visitor);

    void insert(MExpr *expr);

    std::vector<MExpr *> get_exprs();

};

class MBranch : public MExpr {
public:

    MBranch() { }

    MBranch(std::string name) : MExpr(name) { }

    virtual ~MBranch() { }

    virtual void accept(MIRVisitor *visitor) = 0;

};

class MDirectBranch : public MBranch {
private:

    MBlock *branch_to;

public:

    MDirectBranch(MBlock *branch_to) : branch_to(branch_to) { }

    MDirectBranch(std::string name, MBlock *branch_to) : MBranch(name), branch_to(branch_to) { }

    virtual ~MDirectBranch() { }

    void accept(MIRVisitor *visitor);

    MBlock *get_branch_block();

};

class MCondBranch : public MBranch {
private:

    MBlock *branch_to_true;
    MBlock *branch_to_false;
    MVar **condition;

public:

    MCondBranch(MBlock *branch_to_true, MBlock *branch_to_false, MVar **condition) : branch_to_true(branch_to_true),
                                                                                     branch_to_false(branch_to_false),
                                                                                     condition(condition) { }

    MCondBranch(std::string name, MBlock *branch_to_true, MBlock *branch_to_false, MVar **condition) :
            MBranch(name),
            branch_to_true(branch_to_true),
            branch_to_false(branch_to_false),
            condition(condition) { }

    virtual ~MCondBranch() { }

    void accept(MIRVisitor *visitor);

    MBlock *get_branch_block_true();

    MBlock *get_branch_block_false();

    MVar *get_actual_condition();

    MVar **get_condition();

};

// an index into an array
//class MIndex : public IR {
//private:
//
//    MVar array;
//    int index;
//
//public:
//
//    MIndex(MVar array, int index) : array(array), index(index) {
//        assert(array.get_mtype()->is_marray_type() || array.get_mtype()->is_mmatrix_type());
//    }
//
//};

class MBinaryOp : public MExpr {
protected:

    // left and right should be computed by the time they are accessed with get_left and get_right
    MVar **left;
    MVar **right;
    MVar **result;

public:

    MBinaryOp(MVar **left, MVar **right) : left(left), right(right) {
        result = (MVar**)malloc(sizeof(MVar*));
    }

    virtual ~MBinaryOp() { }

    MVar *get_left();

    MVar *get_right();

    // use once value is computed
    MVar *get_actual_result();

    // user when just need a reference to a value that hasn't been completed yet
    MVar **get_result();

    void set_result(MVar *res);

    virtual void accept(MIRVisitor *visitor) = 0;

};

class MAdd : public MBinaryOp {
public:

    MAdd(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MAdd() { }

    void accept(MIRVisitor *visitor);

};

class MSub : public MBinaryOp {
public:

    MSub(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MSub() { }

    void accept(MIRVisitor *visitor);

};

class MMul : public MBinaryOp {
public:

    MMul(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MMul() { }

    void accept(MIRVisitor *visitor);

};

class MDiv : public MBinaryOp {
public:

    MDiv(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MDiv() { }

    void accept(MIRVisitor *visitor);

};

// less than
class MSLT : public MBinaryOp {
public:

    MSLT(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MSLT() { }

    void accept(MIRVisitor *visitor);

};

#endif //MATCHIT_IR_H
