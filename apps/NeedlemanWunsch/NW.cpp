//
// Created by Jessica Ray on 4/5/16.
//

#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Needleman-Wunsch alignment without use of MatchIT

typedef enum {
    NORTHWEST = 0,
    NORTH = 1,
    WEST = 2
} Direction;

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

/**
 * Similarity matrix between two bases.
 * A = 0, G = 1, C = 2, T = 3
 *
 * From https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm "Advanced presentation of algorithm"
 */
//char similarity[4][4] = {{10, -1, -3, -4},
//                         {-1, 7, -5, -3},
//                         {-3, -5, 9, 0},
//                         {-4, -3, 0, 8}};

char similarity[4][4] = {{1, 0, 0, 0},
                         {0, 1, 0, 0},
                         {0, 0, 1, 0},
                         {0, 0, 0, 1}};

int gap_penalty = 0;

inline Direction max_scores_idx(int *scores) {
    if (scores[NORTHWEST] >= scores[NORTH] && scores[NORTHWEST] >= scores[WEST]) {
        return NORTHWEST;
    } else if (scores[NORTH] >= scores[NORTHWEST] && scores[NORTH] >= scores[WEST]) {
        return NORTH;
    } else {
        return WEST;
    }
}

typedef struct {
    std::string name;
    char *sequence;
    int sequence_length;
} Sequence;

/**
 * Compute the scores of a single row in the alignment matrix.
 * Also fills in the first column of the row.
 * Traceback = running alignment. Fill in the (starting_row_index - 1)th entry (-1 because the first row is just gap penalties)
 * The first row is computed in compute_first_row()
 */
void compute_row(char row_base, char *top_sequence, int *upper_row_scores, int *cur_row_scores, int row_size,
                 int starting_row_idx) {
    // compute 3 possible scores and take the max
    // Score 1 (northwest, match): M(i-1,j-1) + S(base_i, base_j)
    // Score 2 (west, deletion): M(i-1,j) + G
    // Score 3 (north, insertion): M(i,j-1) + G
    // initialize the first column
    cur_row_scores[0] = starting_row_idx * gap_penalty;
    for (int col = 1; col < row_size; col++) {
        int northwest_score = upper_row_scores[col-1] + similarity[row_base][top_sequence[col-1]];
        int north_score = upper_row_scores[col] + gap_penalty;
        int west_score = cur_row_scores[col-1] + gap_penalty;
        int scores[3] = {northwest_score, north_score, west_score};
        Direction max_idx = max_scores_idx(scores);
        cur_row_scores[col] = scores[max_idx];
    }
}

/**
 * Fill in first row
 */
void compute_first_row(int *first_row, int row_size) {
    for (int col = 0; col < row_size; col++) {
        first_row[col] = col * gap_penalty;
    }
}

char *compute_traceback_alignment(int **alignment_matrix, Sequence *top_sequence, Sequence *left_sequence) {
    int i = left_sequence->sequence_length;
    int j = top_sequence->sequence_length;
    int max_length = top_sequence->sequence_length > left_sequence->sequence_length ? top_sequence->sequence_length : left_sequence->sequence_length;
    char top_alignment[2*max_length+1];
    char left_alignment[2*max_length];
    int top_idx = 0;
    int left_idx = 0;
    while (i > 0 || j > 0) {
        if (i == 0 && j == 0) {
            top_alignment[top_idx++] = to_letter_base(top_sequence->sequence[j]);
            left_alignment[left_idx++] = to_letter_base(left_sequence->sequence[i]);
            i--;
            j--;
        } else if (i > 0 && j > 0 &&
                alignment_matrix[i][j] == (alignment_matrix[i-1][j-1] + similarity[left_sequence->sequence[i-1]][top_sequence->sequence[j-1]])) {
            // they match
            top_alignment[top_idx++] = to_letter_base(top_sequence->sequence[j-1]);
            left_alignment[left_idx++] = to_letter_base(left_sequence->sequence[i-1]);
            i--;
            j--;
        } else if (i > 0 && alignment_matrix[i][j] == alignment_matrix[i-1][j] + gap_penalty) {
            // left aligns with a gap
            top_alignment[top_idx++] = '-';
            left_alignment[left_idx++] = to_letter_base(left_sequence->sequence[i-1]);
            i--;
        } else {
            // top aligns with a gap
            top_alignment[top_idx++] = to_letter_base(top_sequence->sequence[j-1]);
            left_alignment[left_idx++] = '-';
            j--;
        }
    }
    top_alignment[top_idx] = '\n';
    top_alignment[top_idx+1] = '\0';
    left_alignment[left_idx] = '\0';
    char *alignment = (char*)malloc(sizeof(char) * (top_idx + left_idx - 1));
    strcat(alignment, top_alignment);
    strcat(alignment, left_alignment);
    return alignment;
}

inline std::string get_second_token(std::string to_split, char delimiter) {
    char c[to_split.size() + 1];
    const char *cc = to_split.c_str();
    for (int i = 0; i < to_split.size(); i++) {
        c[i] = cc[i];
    }
    c[to_split.size()] = '\0';
    strtok(c, &delimiter);
    return std::string(strtok(NULL, &delimiter));
}

inline char *get_all_tokens(std::string to_split) {
    char *c = (char*)malloc(sizeof(char) * (to_split.size() + 1));
    for (int i = 0; i < to_split.size(); i++) {
        c[i] = to_split[i];
    }
    c[to_split.size()] = '\0';
    return c;
}

std::vector<Sequence *> read_fasta(std::string filename) {
    std::string line;
    std::ifstream fasta(filename);
    std::vector<Sequence *> sequences;
    int cur_seq = 0;
    if (fasta.is_open()) {
        while (getline(fasta, line)) {
            if (line.c_str()[0] == '>') {
                Sequence *next = new Sequence();
                next->name = get_second_token(line, ' ');
                sequences.push_back(next);
            } else {
                char *tokens = get_all_tokens(line);
                Sequence *cur = sequences[cur_seq++];
                cur->sequence_length = line.size();
                cur->sequence = (char*)malloc(sizeof(char) * (line.size() + 1));
                for (int i = 0; i < line.size(); i++) {
                    cur->sequence[i] = to_digit_base(tokens[i]);
                }
                cur->sequence[line.size()] = '\0';
            }
        }
    }
    fasta.close();
    return sequences;
}

int main() {

    std::vector<Sequence *> sequences = read_fasta("/Users/JRay/Documents/Research/datasets/genome/sequences.fasta");

    Sequence *top = sequences[0];
    Sequence *left = sequences[1];

    std::cerr << "top sequence is " << top->name << " with length " << top->sequence_length << std::endl;
    std::cerr << "left sequence is " << left->name << " with length " << left->sequence_length << std::endl;

    // initialize the alignment matrix
    int **alignment_matrix = (int**)malloc(sizeof(int*) * (left->sequence_length + 1)); // + 1 for the gap penalties in each row
    for (int i = 0; i < left->sequence_length+1; i++) {
        alignment_matrix[i] = (int*)malloc(sizeof(int) * (top->sequence_length + 1));
    }

    // build the matrix
    compute_first_row(alignment_matrix[0], top->sequence_length+1);

    // iterate down through the rows
    for (int i = 1; i < left->sequence_length+1; i++) {
        int *cur_row = alignment_matrix[i];
        int *upper_row = alignment_matrix[i - 1];
        char *top_seq = top->sequence;
        char row_base = left->sequence[i-1]; // the genome base of this row
        compute_row(row_base, top_seq, upper_row, cur_row, top->sequence_length + 1, i);
    }

    char *final_traceback = compute_traceback_alignment(alignment_matrix, top, left);
    char cur_char = final_traceback[0];
    int idx = 0;
    while (cur_char != '\0') { // count how many chars there are
        idx++;
        cur_char = final_traceback[idx++];
    }
    // print out the traceback
    for (int j = idx-2; j > -1; j--) {
        std::cerr << final_traceback[j];
    }

    return 0;
}





