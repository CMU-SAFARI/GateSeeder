#include "tb_driver.hpp"
#include <fstream>
#include <stdio.h>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: Wrong number of arguments\n");
        exit(2);
    }
    ifstream ifs_read, ifs_idx;
    ifs_read.open(argv[1], ifstream::in);
    ifs_idx.open(argv[2], ifstream::in | ifstream::binary);
    vector<out_loc_t> res = drive_sim(ifs_read, ifs_idx);
    return 0;
}
