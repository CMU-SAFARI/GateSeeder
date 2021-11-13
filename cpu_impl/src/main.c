#include "parse.h"
#include "seeding.h"
#include "unistd.h"
#include <time.h>

int main(int argc, char *argv[]) {
	int option;
	exp_loc_v loc;
	index_t idx;
	FILE *fp_dat = NULL;
	FILE *fp_idx = NULL;
	while ((option = getopt(argc, argv, "t:i:")) != -1) {
		switch (option) {
			case 't':
				fp_dat = fopen(optarg, "r");
				if (fp_dat == NULL) {
					fprintf(stderr, "Error: cannot open `%s`\n", optarg);
					return 1;
				}
				parse_dat(fp_dat, &loc);
				fprintf(stderr, "golden model `%s` parsed\n", optarg);
				break;
			case 'i':
				fp_idx = fopen(optarg, "rb");
				if (fp_idx == NULL) {
					fprintf(stderr, "Error: cannot open `%s`\n", optarg);
					return 1;
				}
				parse_index(fp_idx, &idx);
				fprintf(stderr, "index `%s` parsed\n", optarg);
				break;
		}
	}
	if (fp_idx == NULL) {
		fprintf(stderr, "Error: expected index file (`-i`)");
		return 1;
	}
	if (optind >= argc) {
		fputs("Error: expected fastq file\n", stderr);
		return 1;
	}
	FILE *fp_fastq = fopen(argv[optind], "r");
	if (fp_fastq == NULL) {
		fprintf(stderr, "Error: cannot open `%s`\n", argv[optind]);
		return 1;
	}
	read_v reads;
	parse_fastq(fp_fastq, &reads);
	fprintf(stderr, "reads `%s` parsed\n", argv[optind]);
	fputs("\t SEEDING STARTS\n", stderr);
	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);
	read_seeding(idx, reads);
	clock_gettime(CLOCK_MONOTONIC, &finish);
	fputs("\t SEEDING IS OVER\n", stderr);
	fprintf(stderr, "Time: %f sec\n",
	        finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
