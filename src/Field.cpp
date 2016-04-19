//
// Created by Jessica Ray on 3/10/16.
//

#include "./Field.h"
#include "./CodegenUtils.h"

/*
 * BaseField
 */

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

int BaseField::get_data_idx() {
    return 6;
}

/*
 * Element
 */

int Element::get_element_id() const {
    return id;
}

/*
 * Fields
 */

void Fields::add(BaseField *field) {
    fields.push_back(field);
}

std::vector<MType *> Fields::get_mtypes() {
    std::vector<MType *> mtypes;
    for(std::vector<BaseField *>::iterator iter = fields.begin(); iter != fields.end(); iter++) {
        mtypes.push_back((*iter)->get_data_mtype());
    }
    return mtypes;
}

std::vector<BaseField *> Fields::get_fields() {
    return fields;
}

void init_element(JIT *jit) {
    std::vector<llvm::Type *> element_args;
    element_args.push_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));
    MType *setelement_mtype = create_type<Element>();
    llvm::FunctionType *element_ft = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::PointerType::get(setelement_mtype->codegen_type(), 0), 0),
            element_args, false);
    jit->get_module()->getOrInsertFunction("create_elements", element_ft);
}

extern "C" Element **create_elements(int num_to_create) {
    Element **elements = (Element**)malloc(sizeof(Element*) * num_to_create);
    for (int i = 0; i < num_to_create; i++) {
        elements[i] = new Element(i);
    }
    return elements;
}