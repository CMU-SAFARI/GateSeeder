#include "err.h"
#include "extraction.h"
#include "parsing.h"
#include "querying.h"
#include "util.h"
#include <fcntl.h>

unsigned SE_W        = 12;
unsigned SE_K        = 15;
unsigned IDX_B       = 26;
unsigned IDX_MAX_OCC = 500;
unsigned BATCH_SIZE  = 1000;

int main(int argc, char *argv[]) {
	if (argc != 4) {
		errx(1, "[ERROR] Wrong number of arguments");
	}

	index_t index = parse_index(argv[1]);
	int fd        = open(argv[2], O_RDONLY);
	if (fd == -1) {
		err(1, "open %s", argv[2]);
	}
	open_reads(fd);

	read_v input;
	MALLOC(input.reads, read_t, BATCH_SIZE);
	extraction_init();
	seed_v minimizers = {.capacity = 1 << 10};
	MALLOC(minimizers.seeds, seed_t, 1 << 10);
	pos_v key_pos = {.capacity = 1 << 10};
	MALLOC(key_pos.poss, pos_t, 1 << 10);

	parse_reads(&input);

	/*
	printf("MAP\n");
	uint32_t prev = 0;
	for (uint32_t i = 0; i < 200; i++) {
	        if (index.map[i] != prev) {
	                printf("%x: %x\n", i, index.map[i]);
	                prev = index.map[i];
	        }
	}
	*/

	while (input.nb_reads != 0) {
		for (unsigned i = 0; i < input.nb_reads; i++) {
			extract_seeds_mapping(input.reads[i].seq, input.reads[i].len, &minimizers, 0);
#ifdef DEBUG_SEED_EXTRACTION
			for (uint32_t j = 0; j < minimizers.len; j++) {
				printf("hash: %010lx, loc: %u\n", minimizers.seeds[j].hash, minimizers.seeds[j].loc);
			}
#endif
			query_index_map(minimizers, index.map, &key_pos);

#ifdef DEBUG_QUERY_INDEX_MAP
			for (uint32_t j = 0; j < key_pos.len; j++) {
				printf("start_pos: %x, end_pos: %x, seed_id: %x, loc: %x\n", key_pos.poss[j].start_pos,
				       key_pos.poss[j].end_pos, key_pos.poss[j].seed_id, key_pos.poss[j].query_loc);
			}
			puts("");
#endif
		}
		parse_reads(&input);
	}

	return 0;
}
