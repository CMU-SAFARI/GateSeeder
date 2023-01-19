#include "demeter_util.h"
#include "err.h"
#include "extraction.h"
#include "parsing.h"
#include "querying.h"
#include <fcntl.h>

unsigned SE_W         = 19;
unsigned SE_K         = 19;
unsigned IDX_MAP_SIZE = 30;
unsigned IDX_MAX_OCC  = 500;
unsigned BATCH_SIZE   = 1000;

int main(int argc, char *argv[]) {
	if (argc != 3) {
		errx(1, "Usage\t%s <index.dti> <query.fastq>", argv[0]);
	}

	index_t index = index_parse(argv[1]);
	int fd        = open(argv[2], O_RDONLY);
	if (fd == -1) {
		err(1, "open %s", argv[2]);
	}
	open_reads(fd);

	read_v input;
	MALLOC(input.reads, read_t, BATCH_SIZE);
	extraction_init();

	seed_v minimizers = {.capacity = 0, .len = 0, .seeds = NULL};
	pos_v key_pos     = {.capacity = 0, .len = 0, .poss = NULL};
	loc_v loc         = {.capacity = 0, .len = 0, .locs = NULL};

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
#ifdef HASH_DISTRIBUTION
			for (uint32_t j = 0; j < minimizers.len - 1; j++) {
				printf("%ld\n",
				       (int64_t)minimizers.seeds[j].hash - (int64_t)minimizers.seeds[j + 1].hash);
			}
#endif
#ifdef SEED_EXTRACTION
			for (uint32_t j = 0; j < minimizers.len; j++) {
				printf("hash: %010lx, loc: %x\n", minimizers.seeds[j].hash, minimizers.seeds[j].loc);
			}
			printf("Nb_seeds: %u\n", minimizers.len);
#endif
			query_index_map(minimizers, index.map, &key_pos);

#ifdef QUERY_INDEX_MAP
			for (uint32_t j = 0; j < key_pos.len; j++) {
				printf("start_pos: %x, end_pos: %x, seed_id: %x, loc: %x\n", key_pos.poss[j].start_pos,
				       key_pos.poss[j].end_pos, key_pos.poss[j].seed_id, key_pos.poss[j].query_loc);
			}
			puts("");
#endif
			query_index_key(key_pos, index.key, &loc);
#ifdef QUERY_INDEX_KEY
			printf("loc_len: %u\n", loc.len);
			for (uint32_t j = 0; j < loc.len; j++) {
				printf("target_loc: %x, query_loc: %x, chrom_id: %x, str: %x\n", loc.locs[j].target_loc,
				       loc.locs[j].query_loc, loc.locs[j].chrom_id, loc.locs[j].str);
			}
#endif
		}
		parse_reads(&input);
	}
	return 0;
}
