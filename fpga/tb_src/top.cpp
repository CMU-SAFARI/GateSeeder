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

void print_results(uint64_t *results) {
	uint32_t i   = 0;
	uint64_t res = results[0];
	while (res != UINT64_MAX) {
		std::cout << std::hex << i << ": " << res << std::endl;
		i++;
		res = results[i];
	}
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		cerr << "[ERROR] Wrong number of arguments" << endl;
		exit(2);
	}
	index_t index = parse_index(argv[1]);

	/*
	// DEBUG
	printf("MAP\n");
	uint32_t prev = 0;
	for (uint32_t i = 0; i < 200; i++) {
	        if (index.map[i] != prev) {
	                printf("%x: %x\n", i, index.map[i]);
	                prev = index.map[i];
	        }
	}
	*/
	read_buf_t read_buf;
	open_fastq(OPEN_MMAP, argv[2]);
	read_buf_init(&read_buf, 1 << 20);

	std::cout << "Read file open\n";

	uint64_t *results;
	MALLOC(results, uint64_t, 1 << 28);

	uint64_t *out_0_o;
	MALLOC(out_0_o, uint64_t, 1 << 20);
	uint64_t *out_1_o;
	MALLOC(out_1_o, uint64_t, 1 << 20);

	while (parse_fastq(&read_buf) == 0) {
		std::cout << "read buf len: " << read_buf.len << std::endl;
		kernel(read_buf.len, read_buf.seq, index.map, index.key[0], index.key[1], out_0_o, out_1_o);
		print_results(out_0_o);
	}
	kernel(read_buf.len, read_buf.seq, index.map, index.key[0], index.key[1], out_0_o, out_1_o);
	print_results(out_0_o);

	return 0;
}
