#include "indexing.h"
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);
	unsigned int w = 12;
	unsigned int k = 18;
	unsigned int f = 700;
	unsigned int b = 26;
	char p         = 0;
	unsigned int t = 150;
	int option;
	while ((option = getopt(argc, argv, ":w:k:f:b:pt:")) != -1) {
		switch (option) {
			case 'w':
				w = atoi(optarg);
				if (w > 255) {
					fputs("Error: `w` needs to be less than or equal to 255\n", stderr);
					exit(4);
				}
				break;
			case 'k':
				k = atoi(optarg);
				if (k > 28) {
					fputs("Error: `k` needs to be less than or equal to 28\n", stderr);
					exit(4);
				}
				break;
			case 'f':
				f = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				if (b > 32) {
					fputs("Error: `b` needs to be less than or equal to 32\n", stderr);
					exit(4);
				}
				break;
			case 'p':
				p = 1;
				break;
			case 't':
				t = atoi(optarg);
				break;
			case ':':
				fprintf(stderr, "Error: option '%c' requires a value\n", optopt);
				exit(3);
			case '?':
				fprintf(stderr, "Error: unknown option: '%c'\n", optopt);
				exit(3);
		}
	}

	if (optind + 1 >= argc) {
		fputs("Error: USAGE\t indexdna [option]* <DNA FILE> <OUTPUT NAME>\n", stderr);
		exit(3);
	}

	int fd_in = open(argv[optind], O_RDONLY);
	if (fd_in == -1) {
		err(1, "open %s", argv[optind]);
	}

	printf("Info: w = %u, k = %u, f = %u, b = %u & t = %u\n", w, k, f, b, t);
	if (p) {
		puts("Info: Partioning enabled");
		create_index_part(fd_in, w, k, f, b, t, argv[optind + 1]);
	} else {
		char name_buf[200];
		sprintf(name_buf, "%s.bin", argv[optind + 1]);
		FILE *fp_out = fopen(name_buf, "wb");
		if (fp_out == NULL) {
			err(1, "fopen %s", name_buf);
		}
		puts("Info: Partioning disabled");
		index_t idx;
		create_index(fd_in, w, k, f, b, &idx);
		fwrite(&(idx.n), sizeof(idx.n), 1, fp_out);
		fwrite(idx.h, sizeof(idx.h[0]), idx.n, fp_out);
		fwrite(idx.loc, sizeof(idx.loc[0]), idx.m, fp_out);
		printf("Info: Binary file `%s` written\n", name_buf);
	}
	clock_gettime(CLOCK_MONOTONIC, &finish);
	printf("EXECUTION TIME: %f sec\n",
	       finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
