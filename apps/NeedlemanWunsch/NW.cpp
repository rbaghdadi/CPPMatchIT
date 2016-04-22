//
// Created by Jessica Ray on 4/22/16.
//

#include <fstream>
#include "../../src/ComparisonStage.h"
#include "../../src/Pipeline.h"
#include "../../src/StageFactory.h"
#include "../../src/TransformStage.h"
#include "./NW.h"

namespace NW {

// inputs to compute_alignment_matrix
Field<char,max_filepath_size> filter_filepath;
Field<char,max_seq_size> sequence;
Field<char,max_sequence_name> sequence_name;
Field<int> sequence_length;

// outputs of compute_alignment_matrix
Field<int,max_seq_size+1,max_seq_size+1> alignment_matrix;
Field<char,max_sequence_name> sequence1_name;
Field<char,max_seq_size> sequence1;
Field<int> sequence1_length;
Field<char,max_sequence_name> sequence2_name;
Field<char,max_seq_size> sequence2;
Field<int> sequence2_length;

// outputs of compute_traceback_alignment
Field<char, traceback_length> traceback;

char similarity[4][4] = {{10, -1, -3, -4},
{-1, 7, -5, -3},
{-3, -5, 9, 0},
{-4, -3, 0, 8}};

int gap_penalty = -5;

inline char to_digit_base(char letter_base) {
    switch (letter_base) {
        case 'A':
            return 0;
        case 'G':
            return 1;
        case 'C':
            return 2;
        case 'T':
            return 3;
    }
}

inline char to_letter_base(char digit_base) {
    switch (digit_base) {
        case 0:
            return 'A';
        case 1:
            return 'G';
        case 2:
            return 'C';
        case 3:
            return 'T';
        case '-':
            return '-';
        case '\n':
            return '\n';
    }
}

inline Direction max_scores_idx(int *scores) {
    if (scores[NORTHWEST] >= scores[NORTH] && scores[NORTHWEST] >= scores[WEST]) {
        return NORTHWEST;
    } else if (scores[NORTH] >= scores[NORTHWEST] && scores[NORTH] >= scores[WEST]) {
        return NORTH;
    } else {
        return WEST;
    }
}

std::vector<Element *> read_fasta2(std::string fasta_file) {
    std::vector<Element *> elements;
    std::string line;
    std::ifstream fasta(fasta_file);
    assert(fasta && "FASTA file does not exist!");
    int cur_seq = 0;
    if (fasta.is_open()) {
        while (getline(fasta, line)) {
            if (line.c_str()[0] == '>') {
                Element *next = new Element(cur_seq);
                // set the filepath
                next->allocate_and_set(&filter_filepath, &fasta_file[0]);
                // set the sequence name
                std::vector<std::string> tokens = split(line, ' ');
                std::cerr << "reading " << tokens[1] << " from file " << fasta_file << std::endl;
                next->allocate_and_set(&sequence_name, &(tokens[1][0]));//c_name);
                elements.push_back(next);
            } else {
                // set the sequence
                const char *tokens = line.c_str();
                Element *cur = elements[cur_seq++];
                cur->allocate(&sequence);
                for (int i = 0; i < line.size(); i++) {
                    cur->set(&sequence, i, to_digit_base(tokens[i]));
                }
                cur->set(&sequence, line.size(), '\0');
                cur->allocate(&sequence_length);
                cur->set(&sequence_length, (int)line.size());
            }
        }
    }
    fasta.close();
    return elements;
}

extern "C" bool compute_alignment_matrix(const Element * const sequence1_in, const Element * const sequence2_in, Element * const out) {
    std::cerr << "computing the alignment matrix for " << sequence1_in->get(&sequence_name) << " and " << sequence2_in->get(&sequence_name) << std::endl;
    out->set(&sequence1_name, sequence1_in->get(&sequence_name));
    out->set(&sequence2_name, sequence2_in->get(&sequence_name));
    out->set(&sequence1, sequence1_in->get(&sequence));
    out->set(&sequence2, sequence2_in->get(&sequence));
    out->set(&sequence1_length, sequence1_in->get(&sequence_length));
    out->set(&sequence2_length, sequence2_in->get(&sequence_length));
    // + 1 below all of these because there is an extra column and row for the gap penalties
    for (int col = 0; col < sequence1_in->get(&sequence_length) + 1; col++) {
        out->set(&alignment_matrix, 0, col, col * gap_penalty);
    }
    // iterate down through the rows of the alignment matrix
    for (int i = 1; i < sequence2_in->get(&sequence_length) + 1; i++) {
        int *cur_row = out->get(&alignment_matrix, i); // current row computing the score for
        int *upper_row = out->get(&alignment_matrix, i - 1); // previous row computed
        char *top_seq = sequence1_in->get(&sequence); // sequence on the horizontal of the alignment matrix
        char row_base = sequence2_in->get(&sequence)[i-1]; // the genome base of this row
        cur_row[0] = i * gap_penalty;
        for (int col = 1; col < sequence1_in->get(&sequence_length) + 1; col++) {
            int northwest_score = upper_row[col-1] + similarity[row_base][top_seq[col-1]];
            int north_score = upper_row[col] + gap_penalty;
            int west_score = cur_row[col-1] + gap_penalty;
            int scores[3] = {northwest_score, north_score, west_score};
            Direction max_idx = max_scores_idx(scores);
            cur_row[col] = scores[max_idx];
        }
    }
    return true; // keep everything since the traceback alignment will be computed next
}

extern "C" void compute_traceback_alignment(const Element * const in, Element * const out) {
    std::cerr << "Computing traceback for sequences: " << in->get(&sequence1_name) << " and " <<
    in->get(&sequence2_name) << std::endl;
    int i = in->get(&sequence2_length);
    int j = in->get(&sequence1_length);
    int max_length = in->get(&sequence1_length) > in->get(&sequence2_length) ? in->get(&sequence1_length) : in->get(&sequence2_length);
    char top_alignment[2 * max_length];
    char left_alignment[2 * max_length];
    int top_idx = 0;
    int left_idx = 0;
    while (i > 0 || j > 0) {
        if (i == 0 && j == 0) {
            top_alignment[top_idx++] = to_letter_base(in->get(&sequence1, j));
            left_alignment[left_idx++] = to_letter_base(in->get(&sequence2, i));
            i--;
            j--;
        } else if (i > 0 && j > 0 &&
                   in->get(&alignment_matrix, i, j) == (in->get(&alignment_matrix, i-1,j-1) +
                                                        similarity[in->get(&sequence2, i - 1)][in->get(&sequence1, j - 1)])) {
            // top and left align with each other
            top_alignment[top_idx++] = to_letter_base(in->get(&sequence1, j - 1));
            left_alignment[left_idx++] = to_letter_base(in->get(&sequence2, i - 1));
            i--;
            j--;
        } else if (i > 0 && in->get(&alignment_matrix, i, j) == in->get(&alignment_matrix, i - 1, j) + gap_penalty) {
            // left aligns with a gap
            top_alignment[top_idx++] = '-';
            left_alignment[left_idx++] = to_letter_base(in->get(&sequence2, i - 1));
            i--;
        } else {
            // top aligns with a gap
            top_alignment[top_idx++] = to_letter_base(in->get(&sequence1, j - 1));
            left_alignment[left_idx++] = '-';
            j--;
        }
    }
    // combine the tracebacks for both sequences
    char alignment[traceback_length];
    int m = 0;
    for (int n = top_idx - 1; n >= 0; n--) {
        alignment[m++] = top_alignment[n];
    }
    alignment[top_idx] = '\n';
    m = top_idx + 1;
    for (int n = left_idx - 1; n >= 0; n--) {
        alignment[m++] = left_alignment[n];
    }
    out->set(&traceback, alignment);

    // print out the alignment
    for (int n = 0; n < top_idx + left_idx + 1; n++) {
        std::cerr << alignment[n];
    }
    std::cerr << std::endl;
}


void setup(JIT *jit) {
    Fields alignment_matrix_inputs;
    alignment_matrix_inputs.add(&filter_filepath);
    alignment_matrix_inputs.add(&sequence);
    alignment_matrix_inputs.add(&sequence_name);
    alignment_matrix_inputs.add(&sequence_length);

    Fields alignment_matrix_outputs;
    alignment_matrix_outputs.add(&alignment_matrix);
    alignment_matrix_outputs.add(&sequence1_name);
    alignment_matrix_outputs.add(&sequence2_name);
    alignment_matrix_outputs.add(&sequence1);
    alignment_matrix_outputs.add(&sequence2);
    alignment_matrix_outputs.add(&sequence1_length);
    alignment_matrix_outputs.add(&sequence2_length);

    Fields traceback_relation;
    traceback_relation.add(&traceback);

    ComparisonStage compare = create_comparison_stage(jit, compute_alignment_matrix, "compute_alignment_matrix",
                                                      &alignment_matrix_inputs, &alignment_matrix_outputs);

    TransformStage traceback_stage = create_transform_stage(jit, compute_traceback_alignment, "compute_traceback_alignment",
                                                            &alignment_matrix_outputs, &traceback_relation);

    Pipeline pipeline;
    pipeline.register_stage(&compare);
    pipeline.register_stage(&traceback_stage);
    pipeline.codegen(jit);
}

void start(JIT *jit) {
    std::vector<Element *> in_elements = read_fasta2("/Users/JRay/Documents/Research/datasets/genome/sequences.2.fasta");
    jit->dump();
    jit->add_module();
    run(jit, in_elements, &alignment_matrix, &sequence1_name, &sequence2_name, &sequence1, &sequence2,
        &sequence1_length, &sequence2_length, &traceback);
}


}