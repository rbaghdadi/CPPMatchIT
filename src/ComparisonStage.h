#ifndef MATCHIT_COMPARISONBLOCK_H
#define MATCHIT_COMPARISONBLOCK_H

#include <string>
#include "./Stage.h"
#include "./MType.h"

#define name(s) s

class ComparisonStage : public Stage {
private:

    // This version lets you set an output object. If returns false, the output object is removed
    // Only here to enforce that the user's function signature is correct. And for debugging.
    bool (*compareBIO)(const Element * const, const Element * const, Element * const) = nullptr;

    // This version only lets you say these match or they don't
    // Only here to enforce that the user's function signature is correct. And for debugging.
    bool (*compareBI)(const Element * const, const Element * const) = nullptr;

    ForLoop *inner;

public:

    ComparisonStage(bool (*compareBIO)(const Element * const, const Element * const, Element * const),
                    std::string comparison_name, JIT *jit, Fields *input_relation, Fields *output_relation) :
            Stage(jit, "ComparisonStage", comparison_name, input_relation, output_relation,
                  MScalarType::get_bool_type()), compareBIO(compareBIO) {
        // trick compiler into thinking this is used
        (void)(this->compareBIO);
    }

    ComparisonStage(bool (*compareBI)(const Element * const, const Element * const),
                    std::string comparison_name, JIT *jit, Fields *input_relation) :
            Stage(jit, "ComparisonStage", comparison_name, input_relation,
                  MScalarType::get_bool_type()), compareBI(compareBI) {
        // trick compiler into thinking this is used
        (void)(this->compareBI);
    }

    ~ComparisonStage() {
        delete inner;
    }

    bool is_comparison();

    std::vector<llvm::AllocaInst *> preallocate();

    void codegen_main_loop(std::vector<llvm::AllocaInst *> preallocated_space, llvm::BasicBlock *stage_end);

    /**
     * Compute n^2 output elements.
     */
    llvm::AllocaInst *compute_num_output_elements();

    /**
     * Like filter, only keep outputs that return true.
     */
    void handle_user_function_output();

};

#endif //MATCHIT_COMPARISONBLOCK_H

