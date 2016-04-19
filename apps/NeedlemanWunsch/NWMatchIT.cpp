//
// Created by Jessica Ray on 4/6/16.
//

#include <fstream>

#include "../../src/ComparisonStage.h"
#include "../../src/Field.h"
#include "../../src/LLVM.h"
#include "../../src/Pipeline.h"
#include "../../src/StageFactory.h"
#include "../../src/Utils.h"

#include "./NWUtils.h"

// Custom input function. Reads in sequences
// TODO create const char create_type functions
std::vector<SetElement *> read_fasta(std::string fasta_file, Field<char,150> *filepath, Field<char,10> *sequence, Field<char,50> *sequence_name, Field<int> *sequence_length) {
    std::vector<SetElement *> elements;
    std::string line;
    std::ifstream fasta(fasta_file);
    int cur_seq = 0;
    if (fasta.is_open()) {
        while (getline(fasta, line)) {
            if (line.c_str()[0] == '>') {
                SetElement *next = new SetElement(cur_seq);
                // set the filepath
                const char *fpath = fasta_file.c_str();
                char *c_fpath = (char*)malloc_32(sizeof(char) * 150);//fasta_file.size());
                strcpy(c_fpath, fpath);
                next->init(filepath, c_fpath);
                // set the sequence name
                std::string token = get_second_token(line, ' ');
                const char *name = token.c_str();
                char *c_name = (char*)malloc_32(sizeof(char) * 50);//token.size());
                strcpy(c_name, name);
                next->init(sequence_name, c_name);
                elements.push_back(next);
            } else {
                // set the sequence
                char *tokens = get_all_tokens(line);
                SetElement *cur = elements[cur_seq++];
                cur->init_blank(sequence);
                for (int i = 0; i < line.size(); i++) {
                    cur->set(sequence, i, to_digit_base(tokens[i]));
                }
                cur->set(sequence, line.size(), '\0');
                cur->init_blank(sequence_length);
                cur->set(sequence_length, (int)line.size());
            }
        }
    }
    fasta.close();
    return elements;
}

// inputs
Field<char,150> filepath;
Field<char,10> sequence;
Field<char,50> sequence_name;
Field<int> sequence_length;
// compute_alignment_matrix
Field<int,5,5> alignment_matrix;
Field<char,50> alignment_matrix_sequence1_name;
Field<char,50> alignment_matrix_sequence1;
Field<int> sequence1_length;
Field<char,50> alignment_matrix_sequence2_name;
Field<char,50> alignment_matrix_sequence2;
Field<int> sequence2_length;
// compute_traceback_alignment
Field<char, 50> traceback;

int linear(int row, int col) {
    return row * 5 + col;
}

int linear(int row) {;
    return row * 5;
}

/**
 * Comparison function that computes the alignment matrix all at once without any optimizations
 */
extern "C" bool compute_alignment_matrix(const SetElement * const sequence1, const SetElement * const sequence2, SetElement * const out) {
    std::cerr << "computing the alignment matrix for " << sequence1->get(&sequence_name) << " and " << sequence2->get(&sequence_name) << std::endl;
    out->set(&alignment_matrix_sequence1_name, sequence1->get(&sequence_name));
    out->set(&alignment_matrix_sequence2_name, sequence2->get(&sequence_name));
    out->set(&alignment_matrix_sequence1, sequence1->get(&sequence));
    out->set(&alignment_matrix_sequence2, sequence2->get(&sequence));
    out->set(&sequence1_length, sequence1->get(&sequence_length));
    out->set(&sequence2_length, sequence2->get(&sequence_length));
    int* alignment_mtx = out->get(&alignment_matrix);
    for (int col = 0; col < sequence1->get(&sequence_length) + 1; col++) {
        alignment_mtx[linear(0,col)] = col * gap_penalty;
    }
    // iterate down through the rows
    for (int i = 1; i < sequence2->get(&sequence_length)+1; i++) {
        int *cur_row = &alignment_mtx[linear(i)]; // todo, use getarray here
        int *upper_row = &alignment_mtx[linear(i - 1)];
        char *top_seq = sequence1->get(&sequence);
        char row_base = sequence2->get(&sequence)[i-1]; // the genome base of this row
        cur_row[0] = i * gap_penalty;
        for (int col = 1; col < sequence1->get(&sequence_length) + 1; col++) {
            int northwest_score = upper_row[col-1] + similarity[row_base][top_seq[col-1]];
            int north_score = upper_row[col] + gap_penalty;
            int west_score = cur_row[col-1] + gap_penalty;
            int scores[3] = {northwest_score, north_score, west_score};
            Direction max_idx = max_scores_idx(scores);
            cur_row[col] = scores[max_idx];
        }
    }

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            std::cerr << out->get(&alignment_matrix)[linear(i,j)] << " ";
        }
    }
    std::cerr << std::endl;

    return true;
}

extern "C" void compute_traceback_alignment(const SetElement * const in, SetElement * const out) {
    std::cerr << "Computing traceback for sequences: " << in->get(&alignment_matrix_sequence1_name) << " and " <<
    in->get(&alignment_matrix_sequence2_name) << std::endl;
    int i = in->get(&sequence2_length);
    int j = in->get(&sequence1_length);
    char *left_sequence = in->get(&alignment_matrix_sequence2);
    char *top_sequence = in->get(&alignment_matrix_sequence1);
    int *alignment_mtx = in->get(&alignment_matrix);
    int max_length = in->get(&sequence1_length) > in->get(&sequence2_length) ? in->get(&sequence1_length) : in->get(&sequence2_length);
//    int max_length = 5 > 5 ? 5 : 5;
    char top_alignment[2 * max_length + 1];
    char left_alignment[2 * max_length];
    int top_idx = 0;
    int left_idx = 0;
    while (i > 0 || j > 0) {
        if (i == 0 && j == 0) {
            top_alignment[top_idx++] = to_letter_base(top_sequence[j]);
            left_alignment[left_idx++] = to_letter_base(left_sequence[i]);
            i--;
            j--;
        } else if (i > 0 && j > 0 &&
                   alignment_mtx[linear(i,j)] == (alignment_mtx[linear(i-1,j-1)] + similarity[left_sequence[i-1]][top_sequence[j-1]])) {
            // they match
            top_alignment[top_idx++] = to_letter_base(top_sequence[j-1]);
            left_alignment[left_idx++] = to_letter_base(left_sequence[i-1]);
            i--;
            j--;
        } else if (i > 0 && alignment_mtx[linear(i,j)] == alignment_mtx[linear(i-1,j)] + gap_penalty) {
            // left aligns with a gap
            top_alignment[top_idx++] = '-';
            left_alignment[left_idx++] = to_letter_base(left_sequence[i-1]);
            i--;
        } else {
            // top aligns with a gap
            top_alignment[top_idx++] = to_letter_base(top_sequence[j-1]);
            left_alignment[left_idx++] = '-';
            j--;
        }
    }
    top_alignment[top_idx] = '\n';
    top_alignment[top_idx+1] = '\0';
    left_alignment[left_idx] = '\0';
    char *alignment = (char*)malloc_32(sizeof(char) * 50);//(top_idx + left_idx + 1)); 50 = traceback length
    strcat(alignment, top_alignment);
    strcat(alignment, left_alignment);
    out->set(&traceback, alignment);
    // print out for debug
    char cur_char = alignment[0];
    int idx = 0;
    while (cur_char != '\0') { // count how many chars there are
        idx++;
        cur_char = alignment[idx++];
    }
    // print out the traceback
    for (int j = top_idx + left_idx; j > -1; j--) {
        std::cerr << alignment[j];
    }
    std::cerr << std::endl;
}

int main() {

    LLVM::init();
    JIT jit;
    register_utils(&jit);
    init_set_element(&jit);

    Relation alignment_matrix_inputs;
    Relation alignment_matrix_relation;
    Relation traceback_relation;
    alignment_matrix_inputs.add(&filepath);
    alignment_matrix_inputs.add(&sequence);
    alignment_matrix_inputs.add(&sequence_name);
    alignment_matrix_inputs.add(&sequence_length);
    alignment_matrix_relation.add(&alignment_matrix);
    alignment_matrix_relation.add(&alignment_matrix_sequence1_name);
    alignment_matrix_relation.add(&alignment_matrix_sequence2_name);
    alignment_matrix_relation.add(&alignment_matrix_sequence1);
    alignment_matrix_relation.add(&alignment_matrix_sequence2);
    alignment_matrix_relation.add(&sequence1_length);
    alignment_matrix_relation.add(&sequence2_length);
    traceback_relation.add(&traceback);


    std::vector<SetElement *> in_setelements = read_fasta("/Users/JRay/Documents/Research/datasets/genome/sequences.fasta",
                                                          &filepath, &sequence, &sequence_name, &sequence_length);

    ComparisonStage compare = create_comparison_stage(&jit, compute_alignment_matrix, "compute_alignment_matrix",
                                                      &alignment_matrix_inputs, &alignment_matrix_relation);

    TransformStage traceback_stage = create_transform_stage(&jit, compute_traceback_alignment, "compute_traceback_alignment",
                                                      &alignment_matrix_relation, &traceback_relation);

    Pipeline pipeline;
    pipeline.register_stage(&compare);
    pipeline.register_stage(&traceback_stage);
    pipeline.codegen(&jit);
//    jit.dump();
    jit.add_module();

    run(jit, in_setelements, &alignment_matrix, &alignment_matrix_sequence1_name, &alignment_matrix_sequence2_name,
             &alignment_matrix_sequence1, &alignment_matrix_sequence2, &sequence1_length, &sequence2_length, &traceback);

    return 0;
}
