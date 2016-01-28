//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMBLOCK_H
#define MATCHIT_TRANSFORMBLOCK_H

#include "./Block.h"
#include "./CodegenUtils.h"
#include "./MFunc.h"
#include "./MType.h"

template <typename I, typename O>
class TransformBlock : public Block {

private:

    // TODO this doesn't allow structs of structs...or does it?
    std::vector<MType *> input_struct_fields;
    std::vector<MType *> output_struct_fields;
    O (*transform)(I);

public:

    TransformBlock(O (*transform)(I), std::string transform_name, JIT *jit, std::vector<MType *> input_struct_fields = std::vector<MType *>(),
                   std::vector<MType *> output_struct_fields = std::vector<MType *>()) :
            Block(jit, mtype_of<I>(), mtype_of<O>(), transform_name), transform(transform),
            input_struct_fields(input_struct_fields), output_struct_fields(output_struct_fields) {
    }

    void codegen() {
        // TODO what if input type is a struct (see ComparisonBlock)
        MType *ret_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if (output_type_code == mtype_ptr && !output_struct_fields.empty()) {
            ret_type = create_struct_reference_type(output_struct_fields);
        } else if (output_type_code != mtype_struct) {
            ret_type = create_type<O>();
        } else {
            ret_type = create_struct_type(output_struct_fields);
        }

        MType *arg_type;
        // TODO OH MY GOD THIS IS A PAINFUL HACK. The types need to be seriously revamped
        if(input_type_code == mtype_ptr && !input_struct_fields.empty()) {
            arg_type = create_struct_reference_type(input_struct_fields);
        } else if (input_type_code != mtype_struct) {
            arg_type = create_type<I>();
        } else {
            arg_type = create_struct_type(input_struct_fields);
        }

        std::vector<MType *> arg_types;
        arg_types.push_back(arg_type);
        MFunc *func = new MFunc(function_name, "TransformBlock", ret_type, arg_types, jit);
        set_function(func);

        func->codegen_extern_proto();
        func->codegen_extern_wrapper_proto();

        using namespace CodegenUtils;

        llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", func->get_extern_wrapper());
        llvm::BasicBlock *for_cond = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.cond", func->get_extern_wrapper());
        llvm::BasicBlock *for_inc = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.inc", func->get_extern_wrapper());
        llvm::BasicBlock *for_body = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.body", func->get_extern_wrapper());
        llvm::BasicBlock *for_end = llvm::BasicBlock::Create(llvm::getGlobalContext(), "for.end", func->get_extern_wrapper());
        llvm::BasicBlock *store_data = llvm::BasicBlock::Create(llvm::getGlobalContext(), "store", func->get_extern_wrapper());

        // initialize stuff
        // counters
        jit->get_builder().SetInsertPoint(entry);
        FuncComp loop_idx = init_idx(jit);
        FuncComp ret_idx = init_idx(jit);
        FuncComp args = init_function_args(jit, *func);
        // return structure
        FuncComp ret_struct = init_return_data_structure(jit, ret_type, *func, args.get_last());
        jit->get_builder().CreateBr(for_cond);

        // create the for loop parts
        jit->get_builder().SetInsertPoint(for_cond);
        FuncComp loop_condition = check_loop_idx_condition(jit, llvm::cast<llvm::AllocaInst>(loop_idx.get_result(0)), llvm::cast<llvm::AllocaInst>(args.get_last()));
        jit->get_builder().CreateCondBr(loop_condition.get_result(0), for_body, for_end);
        jit->get_builder().SetInsertPoint(for_inc);
        increment_idx(jit, llvm::cast<llvm::AllocaInst>(loop_idx.get_result(0)));
        jit->get_builder().CreateBr(for_cond);

        // build the body
        jit->get_builder().SetInsertPoint(for_body);
        // get all of the data
        llvm::LoadInst *data = jit->get_builder().CreateLoad(args.get_result(0));
        llvm::LoadInst *cur_loop_idx = jit->get_builder().CreateLoad(loop_idx.get_result(0));
        // now get just the element we want to process
        std::vector<llvm::Value *> data_idx;
        data_idx.push_back(cur_loop_idx);
        llvm::Value *data_gep = jit->get_builder().CreateInBoundsGEP(data, data_idx);
        llvm::LoadInst *element = jit->get_builder().CreateLoad(data_gep);
        llvm::AllocaInst *element_alloc = jit->get_builder().CreateAlloca(element->getType());
        llvm::StoreInst *store_element =  jit->get_builder().CreateStore(element, element_alloc);
        std::vector<llvm::Value *> inputs;
        inputs.push_back(element_alloc);
        FuncComp call_res = create_extern_call(jit, *func, inputs);
        jit->get_builder().CreateBr(store_data);

        // store the result
        jit->get_builder().SetInsertPoint(store_data);
        store_extern_result(jit, ret_type, ret_struct.get_result(0), ret_idx.get_result(0), call_res.get_result(0));
        increment_idx(jit, llvm::cast<llvm::AllocaInst>(ret_idx.get_result(0)));
        jit->get_builder().CreateBr(for_inc);

        // return the data
        jit->get_builder().SetInsertPoint(for_end);
        return_data(jit, ret_struct.get_result(0), ret_idx.get_result(0));

        func->verify_wrapper();
    }

    ~TransformBlock() {}
};

#endif //MATCHIT_TRANSFORMBLOCK_H
