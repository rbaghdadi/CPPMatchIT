//
// Created by Jessica Ray on 4/6/16.
//

#include <fstream>

#include "../../src/ComparisonStage.h"
#include "../../src/Field.h"
#include "../../src/LLVM.h"
#include "../../src/Utils.h"

#include "./NWUtils.h"

// Custom input function. Reads in sequences
// TODO create const char create_type functions
std::vector<SetElement *> read_fasta(std::string fasta_file, Field<char,150> *filepath, Field<char,1500> *sequence, Field<char,50> *sequence_name) {
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
	char *c_fpath = (char*)malloc(sizeof(char) * fasta_file.size());
	strcpy(c_fpath, fpath);
	next->set(filepath, c_fpath);
	// set the sequence name
	std::string token = get_second_token(line, ' ');
	const char *name = token.c_str();
	char *c_name = (char*)malloc(sizeof(char) * token.size());
	strcpy(c_name, name);
	next->set(sequence_name, c_name);
	elements.push_back(next);
      } else {
	// set the sequence
	char *tokens = get_all_tokens(line);
	SetElement *cur = elements[cur_seq++];
	for (int i = 0; i < line.size(); i++) {
	  cur->set(sequence, i, to_digit_base(tokens[i]));
	}
	cur->set(sequence, line.size(), '\0');
      }
    }
  }
  fasta.close();
  return elements;
}

Field<char,150> filepath_field;
Field<char,1500> sequence_field;
Field<char,50> sequence_name_field;

/**
 * Comparison function that computes the alignment matrix all at once without any optimizations
 */
// TODO need to create this type of comparison (one that has an output)
extern "C" void compute_alignment_matrix(const SetElement * const sequence1, const SetElement * const sequence2, SetElement * const out) {

}


int main() {

  LLVM::init();
  JIT jit;
  register_utils(&jit);



  return 0;
}
