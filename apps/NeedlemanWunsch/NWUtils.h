//
// Created by Jessica Ray on 4/6/16.
//

#ifndef MATCHIT_NWUTILS_H
#define MATCHIT_NWUTILS_H

#include <string>
#include <sstream>

/**
 * Similarity matrix between two bases.
 * A = 0, G = 1, C = 2, T = 3
 *
 * From https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm "Advanced presentation of algorithm"
 */
char similarity[4][4] = {{10, -1, -3, -4},
                         {-1, 7, -5, -3},
                         {-3, -5, 9, 0},
                         {-4, -3, 0, 8}};

int gap_penalty = -5;

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

/**
 * Convert digit base back to character
 */
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

//inline std::string get_second_token(std::string to_split, char delimiter) {
//    char c[to_split.size() + 1];
//    const char *cc = to_split.c_str();
//    for (int i = 0; i < to_split.size(); i++) {
//        c[i] = cc[i];
//    }
//    c[to_split.size()] = '\0';
//    strtok(c, &delimiter);
//    return std::string(strtok(NULL, &delimiter));
//}
//
//inline char *get_all_tokens(std::string to_split) {
//    char *c = (char*)malloc(sizeof(char) * (to_split.size() + 1));
//    for (int i = 0; i < to_split.size(); i++) {
//        c[i] = to_split[i];
//    }
//    c[to_split.size()] = '\0';
//    return c;
//}

typedef struct {
    std::string name;
    char *sequence;
    int sequence_length;
} Sequence;

//std::vector<Sequence *> read_fasta(std::string filename) {
//    std::string line;
//    std::ifstream fasta(filename);
//    std::vector<Sequence *> sequences;
//    int cur_seq = 0;
//    if (fasta.is_open()) {
//        while (getline(fasta, line)) {
//            if (line.c_str()[0] == '>') {
//                Sequence *next = new Sequence();
//                next->name = get_second_token(line, ' ');
//                sequences.push_back(next);
//            } else {
//                char *tokens = get_all_tokens(line);
//                Sequence *cur = sequences[cur_seq++];
//                cur->sequence_length = line.size();
//                cur->sequence = (char*)malloc(sizeof(char) * (line.size() + 1));
//                for (int i = 0; i < line.size(); i++) {
//                    cur->sequence[i] = to_digit_base(tokens[i]);
//                }
//                cur->sequence[line.size()] = '\0';
//            }
//        }
//    }
//    fasta.close();
//    return sequences;
//}

#endif //MATCHIT_NWUTILS_H
