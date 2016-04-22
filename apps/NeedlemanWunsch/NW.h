//
// Created by Jessica Ray on 4/22/16.
//

#ifndef MATCHIT_NW_H
#define MATCHIT_NW_H

#include "../../src/Field.h"

namespace NW {

/**
 * Similarity matrix between two bases.
 * A = 0, G = 1, C = 2, T = 3
 *
 * From https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm "Advanced presentation of algorithm"
 */
extern char similarity[4][4];

extern int gap_penalty;// = -5;

/**
 * Which direction a value came from in the alignment matrix (used for traceback)
 */
typedef enum {
    NORTHWEST = 0,
    NORTH = 1,
    WEST = 2
} Direction;

/**
 * Convert input base to a digit used for indexing
 */
inline char to_digit_base(char letter_base);

/**
 * Convert digit base back to character
 */
inline char to_letter_base(char digit_base);

/**
 * Get the direction with the highest score
 */
inline Direction max_scores_idx(int *scores);

const int max_seq_size = 2000;
const int max_filepath_size = 150;
const int max_sequence_name = 50;
const int traceback_length = max_seq_size * 2;

// inputs to compute_alignment_matrix
extern Field<char,max_filepath_size> filter_filepath;
extern Field<char,max_seq_size> sequence;
extern Field<char,max_sequence_name> sequence_name;
extern Field<int> sequence_length;

// outputs of compute_alignment_matrix
extern Field<int,max_seq_size+1,max_seq_size+1> alignment_matrix;
extern Field<char,max_sequence_name> sequence1_name;
extern Field<char,max_seq_size> sequence1;
extern Field<int> sequence1_length;
extern Field<char,max_sequence_name> sequence2_name;
extern Field<char,max_seq_size> sequence2;
extern Field<int> sequence2_length;

// outputs of compute_traceback_alignment
extern Field<char, traceback_length> traceback;

// Custom input function. Reads in sequences
std::vector<Element *> read_fasta2(std::string fasta_file);

/**
 * Comparison function that computes the alignment matrix all at once without any optimizations
 */
extern "C" bool compute_alignment_matrix(const Element * const sequence1_in, const Element * const sequence2_in, Element * const out);

extern "C" void compute_traceback_alignment(const Element * const in, Element * const out);

void setup(JIT *jit);

void start(JIT *jit);

}


#endif //MATCHIT_NW_H
