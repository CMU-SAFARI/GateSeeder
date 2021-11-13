#include "parse.h"
#include "seeding.h"
#include "unistd.h"
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
	int option;
	index_t idx;
	FILE *fp_idx;
	char idx_flag = 0;
	char o_flag   = 0;
#ifdef MULTI_THREAD
	FILE *fp_o[NB_THREADS];
#else
#endif
	while ((option = getopt(argc, argv, "i:o:")) != -1) {
		switch (option) {
			case 'i':
				fp_idx = fopen(optarg, "rb");
				if (fp_idx == NULL) {
					fprintf(stderr, "Error: cannot open `%s`\n", optarg);
					return 1;
				}
				parse_index(fp_idx, &idx);
				fprintf(stderr, "index `%s` parsed\n", optarg);
				idx_flag = 1;
				break;
			case 'o':
#ifdef MULTI_THREAD
				for (unsigned i = 0; i < NB_THREADS; i++) {
					char name_buf[200];
					sprintf(name_buf, "%s_%u.dat", optarg, i);
					fp_o[i] = fopen(name_buf, "w");
					if (fp_o[i] == NULL) {
						fprintf(stderr, "Error: cannot open `%s`\n", optarg);
						return 1;
					}
				}
#else
#endif
				o_flag = 1;
				break;
		}
	}
	if (!idx_flag) {
		fputs("Error: expected index file (with `-i` option)\n", stderr);
		return 1;
	}
	if (!o_flag) {
		fputs("Error: expected output file (with `-o` option)\n", stderr);
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

#ifdef MULTI_THREAD
	read_seeding(idx, reads, fp_o);
#else
#endif

	clock_gettime(CLOCK_MONOTONIC, &finish);
	fputs("\t SEEDING IS OVER\n", stderr);
	fprintf(stderr, "Time: %f sec\n",
	        finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
