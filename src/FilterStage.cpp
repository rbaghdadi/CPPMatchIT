//
// Created by Jessica Ray on 3/28/16.
//

#include "./FilterStage.h"

bool FilterStage::is_filter() {
    return true;
}

// pass along Element if should be kept, otherwise, don't do anything with it
void FilterStage::handle_user_function_output() {
    filter_user_function_output();
}

std::vector<llvm::AllocaInst *> FilterStage::preallocate() { // there isn't any new space to allocate for this
    std::vector<llvm::AllocaInst *> preallocated_space;
    return preallocated_space;
}