#include "parsing.h"
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 4) {
		cerr << "[ERROR] Wrong number of arguments" << endl;
		exit(2);
	}
	ifstream ifs_fastq, ifs_idx, ifs_exp;
	read_buf_t read_buf;
	open_fastq(argv[1]);
	read_buf_init(&read_buf, 1 << 30);
	parse_fastq(&read_buf);

	/*
	for (unsigned i = 0; i < read_buf.len; i++) {
	        printf("%c", read_buf.seq[i]);
	}
	puts("");

	for (unsigned i = 0; i < read_buf.nb_seqs; i++) {
	        printf("%s\n", read_buf.seq_name[i]);
	}
	*/

	// ifs_idx.open(argv[2], ifstream::in | ifstream::binary);
	/*
	vector<out_loc_t> res = drive_sim(ifs_read, ifs_idx);
	ifs_exp.open(argv[3], ifstream::in);
	vector<exp_loc_t> exp = parse_data(ifs_exp);
	*/
}
