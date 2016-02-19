//
// Created by Jessica Ray on 1/28/16.
//

#include "./Stage.h"

MFunc *Stage::get_mfunction() {
    return mfunction;
}

void Stage::set_function(MFunc *mfunction) {
    this->mfunction = mfunction;
}

bool Stage::is_filter() {
    return false;
}

bool Stage::is_transform() {
    return false;
}

bool Stage::is_segmentation() {
    return false;
}

bool Stage::is_comparison() {
    return false;
}
