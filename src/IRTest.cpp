//
// Created by Jessica Ray on 5/2/16.
//

#include <llvm/IR/Verifier.h>
#include "./Init.h"
#include "./IR.h"
#include "./LLVM.h"

int main() {

    JIT *jit = init();

    // Create a useless function with MatchIT IR that just does a bunch of arithmetic operations and
    // then calls an external print function that just prints out a solid row of '=' symbols.
    // This is just a proof of concept.

    // One of the external print functions (prints the row of '=' symbols)
    MType *print_ret_val = MScalarType::get_void_type();
    std::vector<MVar *> print_args;
    MFunction print_func("print_sep", print_args, print_ret_val, true);
    MFunctionCall print_call(&print_func);

    // An external print function that prints out an integer
//    MFunction print_int_func("c_fprintf_int", print_ret_val, true);
//    MFunctionCall print_int_call(&print_int_func);

    // The initial arguments into our function. Variable type
    MVar *arg1 = new MVar(MScalarType::get_int_type(), "left");
    MVar *arg2 = new MVar(MScalarType::get_int_type(), "right");
    std::vector<MVar *> args;
    args.push_back(arg1);
    args.push_back(arg2);

    // A constant MVar
    int const_val = 0;
    MVar *constant = new MVar(MScalarType::get_int_type(), &const_val, "constant", true);

    // The return type of our function
    MType *return_type = MScalarType::get_void_type();

    // Our main function
    MFunction mfunc("my_function", args, return_type);

    // The starting block in our function
    MBlock start_block("start");

    // Create some random arithmetic op calls
    MAdd madd(&arg1, &arg2);
    start_block.insert(&madd);

    MSub msub(&constant, madd.get_result());
    start_block.insert(&msub);

    MMul mmul(madd.get_result(), msub.get_result());
    start_block.insert(&mmul);

    MDiv mdiv(madd.get_result(), mmul.get_result());
    start_block.insert(&mdiv);

    // create a direct branch to a new block "direct_block"
    MBlock direct_block("direct_block");
    MDirectBranch dbranch("direct_branch", &direct_block);
    start_block.insert(&dbranch);

    // put another add op in the new block
    MAdd madd2(mdiv.get_result(), mdiv.get_result());
    direct_block.insert(&madd2);

    // signed less than
    MSLT mslt(madd2.get_result(), &constant);
    direct_block.insert(&mslt);

    // create a conditional branch based on the output of the MSLT operation.
    // branch to either "true_block" or "false_block"
    MBlock true_block("true_block");
    MBlock false_block("false_block");
    MCondBranch cbranch(&true_block, &false_block, mslt.get_result());
    direct_block.insert(&cbranch);

    // Print a bunch of horizontal line in each
    // Also add a return statement in each.
    true_block.insert(&print_call);
    true_block.insert(&print_call);
    true_block.insert(&print_call);
    true_block.insert(&print_call);
    MRetVal void_ret;
    true_block.insert(&void_ret);
    false_block.insert(&print_call);
    false_block.insert(&print_call);
    false_block.insert(&print_call);
    false_block.insert(&void_ret);

    // Add the blocks to our function. This is what makes up the function body
    // ORDER MATTERS
    mfunc.insert(&start_block);
    mfunc.insert(&direct_block);
    mfunc.insert(&true_block);
    mfunc.insert(&false_block);

    // Do all the codegen stuff for LLVM
    LLVM::LLVMCodeGenerator codegen(jit);
    codegen.visit(&mfunc);

    // Run it
    llvm::Function *f = mfunc.get_data<llvm::Function>();
    llvm::verifyFunction(*f);
    jit->dump();
    jit->add_module();
    runIRTest("my_function", jit, 100, 100);

    return 0;

}