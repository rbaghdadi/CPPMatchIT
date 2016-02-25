//
// Created by Jessica Ray on 1/28/16.
//

#ifndef MATCHIT_TRANSFORMSTAGE_H
#define MATCHIT_TRANSFORMSTAGE_H

#include "./Stage.h"
#include "./MFunc.h"
#include "./MType.h"
#include "./CodegenUtils.h"

template <typename I, typename O>
class TransformStage : public Stage {

private:

    void (*transform)(const I*, O*);
//    unsigned int transform_size;
//    bool is_fixed_transform_size = true;

public:

    TransformStage(void (*transform)(const I*, O*), std::string transform_name, JIT *jit, MType *param_type,
                   MType *return_type, bool is_fixed_size = false, unsigned int fixed_size = 0) : //, unsigned int transform_size, bool is_fixed_transform_size) :
            Stage(jit, "TransformStage", transform_name, param_type, return_type, MPrimType::get_void_type(),
            is_fixed_size, fixed_size),
            transform(transform) {}//, transform_size(transform_size), is_fixed_transform_size(is_fixed_transform_size) { }

    ~TransformStage() { }

    void init_codegen() {
        mfunction->codegen_extern_proto();
        mfunction->codegen_extern_wrapper_proto();
    }

    bool is_transform() {
        return true;
    }
};

#endif //MATCHIT_TRANSFORMSTAGE_H
