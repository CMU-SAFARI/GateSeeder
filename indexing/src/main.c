#include "indexing.h"
#include "parsing.h"
#include "types.h"
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned NB_THREADS = 12;

int main(int argc, char *argv[]) {
	int option;
	unsigned w        = 12;
	unsigned k        = 15;
	unsigned size_map = 27;
	unsigned size_ms  = 29;
	unsigned max_occ  = 500;
	while ((option = getopt(argc, argv, ":w:k:b:f:s:")) != -1) {
		switch (option) {
			case 'w':
				w = strtoul(optarg, NULL, 10);
				break;
			case 'k':
				k = strtoul(optarg, NULL, 10);
				break;
			case 'b':
				size_map = strtoul(optarg, NULL, 10);
				break;
			case 'f':
				max_occ = strtoul(optarg, NULL, 10);
				break;
			case 's':
				size_ms = 29;
				break;
			case ':':
				errx(1, "option '%c' requires a value", optopt);
			case '?':
				errx(1, "unknown option: '%c'", optopt);
		}
	}

	if (optind + 2 != argc) {
		errx(1, "USAGE\t %s [option]* <target.fna> <index.ali>", argv[0]);
	}

	fprintf(stderr, "[INFO] w: %u, k: %u, size_map: %u, size_ms: %u max_occ: %u\n", w, k, size_map, size_ms,
	        max_occ);

	int fd_target = open(argv[optind], O_RDONLY);
	if (fd_target == -1) {
		err(1, "open %s", argv[optind]);
	}

	FILE *index_fp = fopen(argv[optind + 1], "wb");
	if (index_fp == NULL) {
		err(1, "fopen %s", argv[optind + 1]);
	}

	target_t target = parse_target(fd_target);
	fprintf(stderr, "[INFO] nb_sequences: %u\n", target.nb_sequences);
	index_t index = gen_index(target, w, k, size_map, max_occ);
	fprintf(stderr, "[INFO] map_len: %u (%lu MB), key_len: %u (%lu MB)\n", index.map_len,
	        index.map_len * sizeof(index.map[0]) >> 20, index.key_len, (index.key_len * 8L) >> 20);

	write_gold_index(index_fp, index, target, w, k, size_map, max_occ);
	const uint32_t map_ms_len = 1 << (size_ms - 2);
	const uint32_t key_ms_len = 1 << (size_ms - 3);
	fprintf(stderr, "[INFO] %u MS(s) required for the map array\n",
	        index.map_len / map_ms_len + ((index.map_len % map_ms_len) != 0));
	fprintf(stderr, "[INFO] %u MS(s) required for the key array\n",
	        index.key_len / key_ms_len + ((index.key_len % key_ms_len != 0)));
	target_destroy(target);
	return 0;
}
