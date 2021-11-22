#include "indexing.h"
#include <stdio.h>
#include <stdlib.h>
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
	int option;
	while ((option = getopt(argc, argv, ":w:k:f:b:p")) != -1) {
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

	FILE *in_fp = fopen(argv[optind], "r");
	if (in_fp == NULL) {
		fprintf(stderr, "Error: cannot open `%s`\n", argv[optind]);
		exit(1);
	}

	printf("Info: w = %u, k = %u, f = %u & b = %u\n", w, k, f, b);
	if (p) {
		puts("Info: Partioning enabled");
		index_v idx;
		create_index_part(in_fp, w, k, f, b, &idx);
		for (size_t i = 0; i < idx.n; i++) {
			char name_buf[200];
			sprintf(name_buf, "%s_%lu.bin", argv[optind + 1], i);
			FILE *out_fp = fopen(name_buf, "wb");
			if (out_fp == NULL) {
				fprintf(stderr, "Error: cannot open `%s`\n", argv[optind + 1]);
				exit(1);
			}
			fwrite(&(idx.a[i].n), sizeof(idx.a[i].n), 1, out_fp);
			fwrite(idx.a[i].h, sizeof(idx.a[i].h[0]), idx.n, out_fp);
			fwrite(idx.a[i].loc, sizeof(idx.a[i].loc[0]), idx.a[i].m, out_fp);
			printf("Info: Binary file `%s` written\n", name_buf);
		}
	} else {
		char name_buf[200];
		sprintf(name_buf, "%s.bin", argv[optind + 1]);
		FILE *out_fp = fopen(name_buf, "wb");
		if (out_fp == NULL) {
			fprintf(stderr, "Error: cannot open `%s`\n", argv[optind + 1]);
			exit(1);
		}
		puts("Info: Partioning disabled");
		index_t idx;
		create_index(in_fp, w, k, f, b, &idx);
		fwrite(&(idx.n), sizeof(idx.n), 1, out_fp);
		fwrite(idx.h, sizeof(idx.h[0]), idx.n, out_fp);
		fwrite(idx.loc, sizeof(idx.loc[0]), idx.m, out_fp);
		printf("Info: Binary file `%s` written\n", name_buf);
	}
	clock_gettime(CLOCK_MONOTONIC, &finish);
	printf("EXECUTION TIME: %f sec\n",
	       finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	return 0;
}
