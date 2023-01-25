#include "demeter_util.h"
#include "kernel.hpp"
#include <err.h>
#include <fstream>
#include <iostream>

using namespace std;

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
		errx(1, "Usage\t%s <index.dti> <query.fastq>", argv[0]);
	}
	index_t index = index_parse(argv[1]);

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
	fa_open(OPEN_MMAP, argv[2]);
	read_buf_init(&read_buf, 1 << 20);

	std::cout << "Read file open\n";

	uint64_t *loc_o;
	MALLOC(loc_o, uint64_t, 1 << 20);

	while (fa_parse(&read_buf) == 0) {
		std::cout << "read buf len: " << read_buf.len << std::endl;
		demeter_kernel(read_buf.len, read_buf.seq, index.map, index.key, loc_o);
#if !defined(DEBUG_QUERY_INDEX_MAP) && !defined(DEBUG_QUERY_INDEX_KEY) && !defined(DEBUG_SEED_EXTRACTION)
		print_results(loc_o);
#endif
	}
	demeter_kernel(read_buf.len, read_buf.seq, index.map, index.key, loc_o);
#if !defined(DEBUG_QUERY_INDEX_MAP) && !defined(DEBUG_QUERY_INDEX_KEY) && !defined(DEBUG_SEED_EXTRACTION)
	print_results(loc_o);
#endif

	return 0;
}
