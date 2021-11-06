#include "tb_driver.hpp"
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 4) {
		cerr << "Error: Wrong number of arguments" << endl;
		exit(2);
	}
	ifstream ifs_read, ifs_idx, ifs_exp;
	ifs_read.open(argv[1], ifstream::in);
	ifs_idx.open(argv[2], ifstream::in | ifstream::binary);
	vector<out_loc_t> res = drive_sim(ifs_read, ifs_idx);
	ifs_exp.open(argv[3], ifstream::in);
	vector<exp_loc_t> exp = parse_data(ifs_exp);
	if (res.size() != exp.size()) {
		cout << "MISMATCH: Not the same number of outputs" << endl;
		return 1;
	}
	for (size_t i = 0; i < res.size(); i++) {
		if (res[i].n != exp[i].n) {
			cout << "MISMATCH: Not the same number of locations: " << res[i].name << " " << res[i].n
			     << "!=" << exp[i].n << endl;
			return 1;
		}
		for (size_t j = 0; j < res[i].n; j++) {
			if (res[i].locs[j] != exp[i].locs[j]) {
				cout << "MISMATCH: wrong location: " << res[i].name << " " << res[i].locs[j]
				     << "!=" << exp[i].locs[j] << endl;
				// return 1;
			}
		}
	}
	cout << "0 MISMATCHES" << endl;
	return 0;
}
