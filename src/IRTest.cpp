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
    std::vector<MVar **> print_args;
    MFunction print_func("print_sep", print_args, print_ret_val, true);
    MFunctionCall print_call(&print_func);

    // An external print function that prints out an integer
    MFunction print_int_func("c_fprintf", print_ret_val, true);
    MFunctionCall print_int_call(&print_int_func);

    // The initial arguments into our function. Variable type
    MVar *arg1 = new MVar(MScalarType::get_int_type(), "left");
    MVar *arg2 = new MVar(MScalarType::get_int_type(), "right");
    std::vector<MVar **> args;
    args.push_back(&arg1);
    args.push_back(&arg2);

    // A function return value
    MRetVal void_ret;

    // A constant MVar
    int const_val = 0;
    MVar *constant = new MVar(MScalarType::get_int_type(), &const_val, "constant", true);

    // The return type of our function
    MType *return_type = MScalarType::get_void_type();

    // Our main function
    MFunction mfunc("my_function", args, return_type);

    // The starting block in our function (well, after a default entry block that is automatically created within the function)
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

    // Create the components of a for loop
    int _loop_start = 0;
    int _loop_bound = 5;
    int _step_size = 1;
    MVar *loop_start = new MVar(MScalarType::get_int_type(), &_loop_start, true);
    MVar *loop_bound = new MVar(MScalarType::get_int_type(), &_loop_bound, true);
    MVar *step_size = new MVar(MScalarType::get_int_type(), &_step_size, true);
    MVar *loop_index = new MVar(MScalarType::get_int_type());
    MBlock body_block("loop_body");
    MBlock loop_after_block("loop_after");
    // the body of the for loop will have a single print statement that prints the result of the previous madd
    print_args.push_back(madd.get_result());
    print_int_func.add_args(print_args);
    body_block.insert(&print_int_call);
    MFor mfor(&loop_start, &loop_bound, &step_size, &loop_index, &body_block, &loop_after_block);
    start_block.insert(&mfor); // add the for loop to our current block

    // now, the next block to work off of is the loop_after_block since that is what goes after the for loop is done.
    // let's make an if/then/else block
    // first get a condition to branch on
    MAdd madd2(mdiv.get_result(), mdiv.get_result());
    loop_after_block.insert(&madd2);
    MSLT mslt(madd2.get_result(), &constant);
    loop_after_block.insert(&mslt);
    // now create the block that we will go to if the condition is true (if)
    MBlock true_block("true_block");
    true_block.insert(&print_call); // just a bunch of print calls that print out lines of '=' symbols
    true_block.insert(&print_call);
    true_block.insert(&print_call);
    true_block.insert(&print_call);
    true_block.insert(&void_ret);
    // and the block we go to if the condition is false (else)
    MBlock false_block("false_block");
    false_block.insert(&print_call);
    false_block.insert(&print_call);
    false_block.insert(&print_call);
    false_block.insert(&void_ret);
    // we have that both blocks return, so there is no 'after' block, hence the nullptr.
    // if we wanted to do something after, we'd create an after block like we did for the for loop
    MIfThenElse mite(mslt.get_result(), &true_block, &false_block, nullptr, nullptr);
    loop_after_block.insert(&mite);

    // Add the outer blocks to our function. This is what makes up the function body
    mfunc.insert(&start_block);
    mfunc.insert(&loop_after_block); // this block is NOT compiled in the for loop, so we need to add it
    // if the if/then/else did stuff after, you'd need to add that block here

    // Do all the codegen stuff for LLVM
    LLVM::LLVMCodeGenerator codegen(jit);
    codegen.visit(&mfunc);

    // Run it
    llvm::Function *f = mfunc.get_data<llvm::Function>();
    llvm::verifyFunction(*f);
    jit->dump();
    jit->add_module();
    runIRTest("my_function", jit, 36, 74);

    return 0;

}