#ifndef MATCHIT_COMPARISONBLOCK_H
#define MATCHIT_COMPARISONBLOCK_H

#include <string>
#include "./Stage.h"
#include "./MType.h"

class ComparisonStage : public Stage {
private:

    // This version lets you set an output object. If returns false, the output object is removed
    bool (*compareBIO)(const SetElement * const, const SetElement * const, SetElement * const);

    // This version only lets you say these match or they don't
    bool (*compareBI)(const SetElement * const, const SetElement * const);

public:

    ComparisonStage(bool (*compareBIO)(const SetElement * const, const SetElement * const, SetElement * const),
                    std::string comparison_name, JIT *jit, Relation *input_relation, Relation *output_relation) :
            Stage(jit, "ComparisonStage", comparison_name, input_relation, output_relation,
                  MScalarType::get_bool_type()), compareBIO(compareBIO) { }

    ComparisonStage(bool (*compareBI)(const SetElement * const, const SetElement * const),
                    std::string comparison_name, JIT *jit, Relation *input_relation) :
            Stage(jit, "ComparisonStage", comparison_name, input_relation,
                  MScalarType::get_bool_type()), compareBI(compareBI) { }

    bool is_comparison();

    std::vector<llvm::AllocaInst *> preallocate();

    llvm::CallInst *codegen_main_loop(std::vector<llvm::AllocaInst *> preallocated_space, llvm::BasicBlock *stage_end);

};

#endif //MATCHIT_COMPARISONBLOCK_H
