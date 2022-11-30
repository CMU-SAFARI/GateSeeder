#include "kernel.hpp"
#include "parsing.h"
#include "util.h"
#include <fstream>
#include <iostream>

using namespace std;

unsigned SE_W        = 12;
unsigned SE_K        = 15;
unsigned IDX_B       = 26;
unsigned IDX_MAX_OCC = 500;
unsigned MS_SIZE     = 1 << 28;

int main(int argc, char *argv[]) {
	if (argc != 4) {
		cerr << "[ERROR] Wrong number of arguments" << endl;
		exit(2);
	}
	index_t index = parse_index(argv[1]);
	read_buf_t read_buf;
	open_fastq(argv[2]);
	read_buf_init(&read_buf, 1 << 30);

	uint64_t *results;
	MALLOC(results, uint64_t, 1 << 30);

	while (parse_fastq(&read_buf) == 0) {
		kernel(read_buf.len, read_buf.seq, index.map, index.key[0], index.key[1], results);
	}
	kernel(read_buf.len, read_buf.seq, index.map, index.key[0], index.key[1], results);

	return 0;
}
