//
// Created by Jessica Ray on 3/10/16.
//

#include "./CodegenUtils.h"
#include "./Field.h"

/*
 * BaseField
 */

MType *BaseField::get_data_mtype() {
    return data_mtype;
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
    return 5;
}

/*
 * Element
 */

int Element::get_element_id() const {
    return id;
}

void Element::set_element_id(int id) {
    this->id = id;
}

void init_element(JIT *jit) {
    std::vector<llvm::Type *> element_args;
    element_args.push_back(Codegen::llvm_int32);
    element_args.push_back(Codegen::llvm_int1);
    MType *setelement_mtype = create_type<Element>();
    llvm::FunctionType *element_ft = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::PointerType::get(setelement_mtype->codegen_type(), 0), 0),
            element_args, false);
    jit->get_module()->getOrInsertFunction("create_elements", element_ft);
}

extern "C" Element **create_elements(int num_to_create, bool alloc_pointers_only) {
#ifdef PRINT_MALLOC
    std::cerr << "creating elements" << std::endl;
#endif
    Element **elements = (Element**)mmalloc(sizeof(Element*) * num_to_create);
    if (!alloc_pointers_only) {
        for (int i = 0; i < num_to_create; i++) {
            Element *new_element = (Element*)mmalloc(sizeof(Element));
            new_element->set_element_id(i);
            elements[i] = new_element;
        }
    }
    return elements;
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
