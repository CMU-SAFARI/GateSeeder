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
	unsigned w       = 12;
	unsigned k       = 15;
	unsigned b       = 26;
	unsigned max_occ = 500;
	while ((option = getopt(argc, argv, ":w:k:b:f")) != -1) {
		switch (option) {
			case 'w':
				w = strtoul(optarg, NULL, 10);
				break;
			case 'k':
				k = strtoul(optarg, NULL, 10);
				break;
			case 'b':
				b = strtoul(optarg, NULL, 10);
				break;
			case 'f':
				max_occ = strtoul(optarg, NULL, 10);
				break;
			case ':':
				errx(1, "option '%c' requires a value", optopt);
			case '?':
				errx(1, "unknown option: '%c'", optopt);
		}
	}

	if (optind + 2 != argc) {
		errx(1, "USAGE\t %s [option]* <target.fna> <index.idx>", argv[0]);
	}

	fprintf(stderr, "[INFO] w: %u, k: %u, b: %u, max_occ: %u\n", w, k, b, max_occ);

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
	index_t index = gen_index(target, w, k, b, max_occ);
	write_index(index_fp, index, target, w, k, b, max_occ);
	index_destroy(index);
	return 0;
}
