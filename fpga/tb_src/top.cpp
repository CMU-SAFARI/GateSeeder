#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 4) {
		cerr << "Error: Wrong number of arguments" << endl;
		exit(2);
	}
	ifstream ifs_fastq, ifs_idx, ifs_exp;
	ifs_fastq.open(argv[1], ifstream::in);
	parse_fastq(ifs_fastq);
	// ifs_idx.open(argv[2], ifstream::in | ifstream::binary);
	/*
	vector<out_loc_t> res = drive_sim(ifs_read, ifs_idx);
	ifs_exp.open(argv[3], ifstream::in);
	vector<exp_loc_t> exp = parse_data(ifs_exp);
	*/
}
