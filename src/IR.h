//
// Created by Jessica Ray on 4/29/16.
//

#ifndef MATCHIT_IR_H
#define MATCHIT_IR_H

#include "./MType.h"

class MVar;
class MFunction;
class MFunctionCall;
class MFor;
class MRetVal;
class MBlock;
class MIfThenElse;
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
    virtual void visit(MFor *mfor) = 0;
    virtual void visit(MFunction *mfunction) = 0;
    virtual void visit(MFunctionCall *mfunction_call) = 0;
    virtual void visit(MRetVal *mret_val) = 0;
    virtual void visit(MBlock *mblock) = 0;
    virtual void visit(MIfThenElse *mif_then_else) = 0;
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
    void *data = nullptr; // use to hold stuff to pass around nodes (if needed)
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

    bool is_data_null();

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

/*
 * MExpr
 */

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

    std::vector<MVar **> args; // TODO this needs to be std::vector<MVar **> so that I can pass in not-yet-created args
    MType *return_type;
    bool var_args;
    std::vector<MBlock *> body;
    bool prototype_only;

public:

    MFunction(std::string name, MType *return_type) : IR(name), return_type(return_type), prototype_only(false) { }

    MFunction(std::string name, MType *return_type, bool prototype_only) : IR(name), return_type(return_type),
                                                                           prototype_only(prototype_only) { }

    MFunction(std::string name, std::vector<MVar **> args, MType *return_type) : IR(name), args(args),
                                                                                 return_type(return_type),
                                                                                 var_args(false),
                                                                                 prototype_only(false) { }

    MFunction(std::string name, std::vector<MVar **> args, MType *return_type, bool prototype_only) :
            IR(name), args(args), return_type(return_type), var_args(false), prototype_only(prototype_only) { }

//    MFunction(std::string name, std::vector<MVar *> args, MType *return_type, bool var_args) : IR(name), args(args),
//                                                                                               return_type(return_type),
//                                                                                               var_args(var_args),
//                                                                                               prototype_only(false) { }

    virtual ~MFunction() { }

    std::vector<MVar **> get_args();

    MType *get_return_type();

    bool is_var_args();

    void insert(MBlock *block);

    std::vector<MBlock *> get_body();

    void accept(MIRVisitor *visitor);

    bool is_prototype_only();

    void add_args(std::vector<MVar **> args);

};

/*
 * MFor
 */

class MFor : public MExpr {
private:

    MVar **start;
    MVar **loop_bound;
    MVar **step_size;
    MVar **loop_index;
    MBlock *body_block; // created outside
    MBlock *end_block; // stuff after the for loop

public:

    MFor(MVar **start, MVar **loop_bound, MVar **step_size, MVar **loop_index, /*MBlock *counter_block,*/
         MBlock *body_block, MBlock *end_block) : start(start), loop_bound(loop_bound), step_size(step_size),
                                                  loop_index(loop_index), /*counter_block(counter_block),*/
                                                  body_block(body_block), end_block(end_block) { }

    virtual ~MFor() { }

    MVar *get_start();

    MVar *get_loop_bound();

    MVar *get_step_size();

    MVar *get_loop_index();

    MBlock *get_body_block();

    MBlock *get_end_block();

    void accept(MIRVisitor *visitor);

};

/*
 * MIfThenElse
 */

class MIfThenElse : public MExpr {
private:

    MVar **condition;
    MBlock *if_mblock;
    MBlock *else_mblock;
    MBlock *after_if_mblock; // allows the if branch to go somewhere after the if/else
    MBlock *after_else_mblock; // allows the else branch to go somewhere after the if/else
    // if after_if==after_else, then this basically joins the two branches back together
    // both should not be set to different blocks, because that is just like them doing different things in their
    // body, so it should jsut be handled internally in the if_mblock and else_mblock


public:

    MIfThenElse(MVar **condition, MBlock *if_mblock, MBlock *else_mblock, MBlock *after_if_mblock,
                MBlock *after_else_mblock) : condition(condition), if_mblock(if_mblock), else_mblock(else_mblock),
                                            after_if_mblock(after_if_mblock), after_else_mblock(after_else_mblock) {
        if (after_if_mblock != after_else_mblock) {
            assert(after_if_mblock == nullptr || after_else_mblock == nullptr);
        }
    }

    MBlock *get_if_mblock();

    MBlock *get_else_mblock();

    MVar *get_actual_condition();

    MBlock *get_after_if_mblock();

    MBlock *get_after_else_mblock();

    void accept(MIRVisitor *visitor);

};

/*
 * MFunctionCall
 */

class MFunctionCall : public MExpr {
private:

    MFunction *mfunction;

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

    MVar *ret_val; // TODO make this a ptr-to-ptr so it can accept not-yet computed MVars

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

/*
 * MBinaryOp
 */

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

/*
 * MAdd
 */

class MAdd : public MBinaryOp {
public:

    MAdd(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MAdd() { }

    void accept(MIRVisitor *visitor);

};

/*
 * MSub
 */

class MSub : public MBinaryOp {
public:

    MSub(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MSub() { }

    void accept(MIRVisitor *visitor);

};

/*
 * MMul
 */

class MMul : public MBinaryOp {
public:

    MMul(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MMul() { }

    void accept(MIRVisitor *visitor);

};

/*
 * MDiv
 */

class MDiv : public MBinaryOp {
public:

    MDiv(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MDiv() { }

    void accept(MIRVisitor *visitor);

};

/*
 * MSLT
 */

// less than
class MSLT : public MBinaryOp {
public:

    MSLT(MVar **left, MVar **right) : MBinaryOp(left, right) { }

    virtual ~MSLT() { }

    void accept(MIRVisitor *visitor);

};

#endif //MATCHIT_IR_H
