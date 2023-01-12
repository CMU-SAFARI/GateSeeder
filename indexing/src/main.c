#include "indexing.h"
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

unsigned NB_THREADS = 12;

int main(int argc, char *argv[]) {
	int option;
	uint32_t w        = 10;
	uint32_t k        = 15;
	uint32_t size_map = 27;
	uint32_t size_ms  = 29;
	uint32_t max_occ  = 500;
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
				size_ms = strtoul(optarg, NULL, 10);
				;
				break;
			case ':':
				errx(1, "option '%c' requires a value", optopt);
			case '?':
				errx(1, "unknown option: '%c'", optopt);
		}
	}

	if (optind + 2 != argc) {
		fprintf(stderr, "Usage %s [OPTION...] <target.fasta> <index.dti>\n", argv[0]);
		exit(1);
	}

	fprintf(stderr, "[INFO] w: %u, k: %u, size_map: %u, max_occ: %u size_ms: %u\n", w, k, size_map, max_occ,
	        size_ms);

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	index_gen(w, k, size_map, max_occ, size_ms, argv[optind], argv[optind + 1]);
	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stderr, "[INFO] Total execution time %f sec\n",
	        end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0);
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	fprintf(stderr, "[INFO] Peak RSS: %f GB\n", r.ru_maxrss / 1048576.0);
	return 0;
}
