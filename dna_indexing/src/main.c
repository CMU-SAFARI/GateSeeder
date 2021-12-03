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
					errx(1, "`w` needs to be less than or equal to 255");
				}
				break;
			case 'k':
				k = atoi(optarg);
				if (k > 28) {
					errx(1, "`k` needs to be less than or equal to 28");
				}
				break;
			case 'f':
				f = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				if (b > 32) {
					errx(1, "``b` needs to be less than or equal to 32");
				}
				break;
			case 'p':
				p = 1;
				break;
			case 't':
				t = atoi(optarg);
				break;
			case ':':
				errx(1, "option '%c' requires a value", optopt);
			case '?':
				errx(1, "unknown option: '%c'", optopt);
		}
	}

	if (optind + 1 >= argc) {
		errx(1, "USAGE\t indexdna [option]* <DNA FILE> <OUTPUT NAME>");
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
		puts("Info: Partioning disabled");
		create_index(fd_in, w, k, f, b, argv[optind + 1]);
	}
	clock_gettime(CLOCK_MONOTONIC, &finish);
	printf("EXECUTION TIME: %f sec\n",
	       finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
