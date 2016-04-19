//
// Created by Jessica Ray on 3/10/16.
//

#include "./Field.h"
#include "./ForLoop.h"
#include "./CodegenUtils.h"

/*
 * BaseField
 */

void BaseField::set_idx(int idx) {
    this->idx = idx;
}

int BaseField::get_idx() {
    return idx;
}

int BaseField::get_dim1() {
    return dim1;
}

int BaseField::get_dim2() {
    return dim2;
}

MType *BaseField::get_data_mtype() {
    return data_mtype;
}

int BaseField::get_and_increment_ctr() {
    return element_ctr++;
}

int BaseField::get_cur_length() {
    return cur_length;
}

int BaseField::get_fixed_size() {
    if (dim1 == 0 && dim2 == 0) {
        return 1;
    } else if (dim2 == 0) {
        return dim1;
    } else {
        return dim1 * dim2;
    }
}

/*
 * SetElement
 */

int SetElement::get_element_id() const {
    return id;
}

/*
 * Relation
 */

void Relation::add(BaseField *field) {
    field->set_idx(fields.size());
    fields.push_back(field);
}

std::vector<MType *> Relation::get_mtypes() {
    std::vector<MType *> mtypes;
    for(std::vector<BaseField *>::iterator iter = fields.begin(); iter != fields.end(); iter++) {
        mtypes.push_back((*iter)->get_data_mtype());
    }
    return mtypes;
}

std::vector<BaseField *> Relation::get_fields() {
    return fields;
}

void init_set_element(JIT *jit) {
    std::vector<llvm::Type *> setelement_args;
    setelement_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    MType *setelement_mtype = create_type<SetElement>();
    llvm::FunctionType *setelement_ft = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::PointerType::get(setelement_mtype->codegen_type(), 0), 0),
            setelement_args, false);
    jit->get_module()->getOrInsertFunction("create_setelements", setelement_ft);
}

extern "C" SetElement **create_setelements(int num_to_create) {
    SetElement **elements = (SetElement**)malloc(sizeof(SetElement*) * num_to_create);
    for (int i = 0; i < num_to_create; i++) {
        elements[i] = new SetElement(i);
    }
    return elements;
}