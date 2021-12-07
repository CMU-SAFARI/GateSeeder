#include "parse.h"
#include "seeding.h"
#include "unistd.h"
#include <err.h>
#include <fcntl.h>
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
	FILE *fp_o;
#endif
	while ((option = getopt(argc, argv, "i:o:")) != -1) {
		switch (option) {
			case 'i':
				fp_idx = fopen(optarg, "rb"); // TODO use open and mmap
				if (fp_idx == NULL) {
					fprintf(stderr, "Error: cannot open `%s`\n", optarg);
					return 1;
				}
				parse_index(fp_idx, &idx);
				printf("Info: index `%s` parsed\n", optarg);
				idx_flag = 1;
				break;
			case 'o':
				strcpy(fp_name, optarg);
#ifdef MULTI_THREAD
				for (unsigned i = 0; i < NB_THREADS; i++) {
					char name_buf[200];
					sprintf(name_buf, "%s_%u.dat", fp_name, i);
					fp_o[i] = fopen(name_buf, "w");
					if (fp_o[i] == NULL) {
						err(1, "fopen %s", name_buf);
					}
				}
#else
				char name_buf[200];
				sprintf(name_buf, "%s.dat", fp_name);
				fp_o = fopen(name_buf, "w");
				if (fp_o == NULL) {
					err(1, "fopen %s", name_buf);
				}
#endif
				o_flag = 1;
				break;
		}
	}

	if (optind >= argc || !o_flag || !idx_flag) {
		errx(1, "USAGE:\t seeding -i <INDEX FILE> -o <OUTPUT NAME> <READS FILE>");
	}
	int fd_reads = open(argv[optind], O_RDONLY);
	if (fd_reads == -1) {
		err(1, "open %s", argv[optind]);
	}
	read_v reads;
	parse_reads(fd_reads, &reads);
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
	printf("\t * %s.dat written\n", fp_name);
#endif
	puts("\t SEEDING FINISHED");
	printf("EXECUTION TIME: %f sec\n",
	       finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
