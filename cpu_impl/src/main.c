#include "parse.h"
#include "seeding.h"
#include "unistd.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
	int option;
	index_t idx;
	FILE *fp_idx;
	char idx_flag = 0;
	char o_flag   = 0;
	char fp_name[200];
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
				printf("Info: index `%s` parsed\n", optarg);
				idx_flag = 1;
				break;
			case 'o':
#ifdef MULTI_THREAD
				for (unsigned i = 0; i < NB_THREADS; i++) {
					char name_buf[200];
					strcpy(fp_name, optarg);
					sprintf(name_buf, "%s_%u.dat", fp_name, i);
					fp_o[i] = fopen(name_buf, "w");
					if (fp_o[i] == NULL) {
						fprintf(stderr, "Error: cannot open `%s`\n", fp_name);
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
	printf("Info: reads `%s` parsed\n", argv[optind]);
	puts("\t SEEDING STARTS");
	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);
	read_seeding(idx, reads, fp_o);
	clock_gettime(CLOCK_MONOTONIC, &finish);
#ifdef MULTI_THREAD
	for (unsigned i = 0; i < NB_THREADS; i++) {
		printf("\t * %s_%u.dat written\n", fp_name, i);
	}
#else
#endif

	clock_gettime(CLOCK_MONOTONIC, &finish);
	puts("\t SEEDING FINISHED");
	printf("Time: %f sec\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
