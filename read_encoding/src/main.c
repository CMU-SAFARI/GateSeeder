#include "encoding.h"
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);
	int option;
	unsigned l = 0;
	while ((option = getopt(argc, argv, ":l:")) != -1) {
		switch (option) {
			case 'l':
				l = atoi(optarg);
				if (!l) {
					errx(1, "`l` needs to be greater than 0");
				}
				break;
			case ':':
				errx(1, "option '%c' requires a value", optopt);
			case '?':
				errx(1, "unknown option: '%c'", optopt);
		}
	}

	if (optind + 1 >= argc) {
		errx(1, "USAGE\t fastq2bin [option]* <FASTQ FILE> <OUTPUT NAME>");
	}

	int fd_in = open(argv[optind], O_RDONLY);
	if (fd_in == -1) {
		err(1, "open %s", argv[optind]);
	}

	if (l) {
		encode_const_len(fd_in, l, argv[optind + 1]);
	} else {
		encode_var_len(fd_in, argv[optind + 1]);
	}

	clock_gettime(CLOCK_MONOTONIC, &finish);
	printf("EXECUTION TIME: %f sec\n",
	       finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
