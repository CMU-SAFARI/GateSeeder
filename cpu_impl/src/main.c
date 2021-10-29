#include "compare.h"
#include "index.h"
#include "parse.h"
#include "seeding.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	unsigned int w        = 12;
	unsigned int k        = 18;
	unsigned int f        = 500;
	unsigned char p       = 0;
	unsigned int b        = 26;
	unsigned char r       = 0;
	unsigned int m        = 4;
	unsigned int l        = 200;
	unsigned char compact = 0;
	cindex_t idx          = {.n = 0, .m = 0, .h = NULL, .location = NULL};
	target_v target       = {.n = 0, .a = NULL};

	int option;
	while ((option = getopt(argc, argv, ":w:k:f:pb:ri:c:m:l:o")) != -1) {
		switch (option) {
			case 'w':
				w = atoi(optarg);
				break;
			case 'k':
				k = atoi(optarg);
				break;
			case 'f':
				f = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				if (b > 32) {
					fputs("Error: 'b' needs to be smaller than 32\n", stderr);
					exit(4);
				}
				break;
			case 'p':
				p = 1;
				break;
			case 'r':
				r = 1;
				break;
			case 'i': {
				FILE *bin_fp = fopen(optarg, "rb");
				if (bin_fp == NULL) {
					fprintf(stderr, "Error: cannot open `%s`\n", optarg);
					exit(1);
				}
				read_cindex(bin_fp, &idx);
				// printf("Info: Binary file: %s read\n", optarg);
			} break;
			case 'c': {
				FILE *paf_fp = fopen(optarg, "r");
				if (paf_fp == NULL) {
					fprintf(stderr, "Error: cannot open `%s`\n", optarg);
					exit(1);
				}
				parse_paf(paf_fp, &target);
				printf("Info: Target file: %s read\n", optarg);

			} break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'l':
				l = atoi(optarg);
				break;
			case 'o':
				compact = 1;
				break;
			case ':':
				fprintf(stderr, "Error: '%c' requires a value\n", optopt);
				exit(3);
			case '?':
				fprintf(stderr, "Error: unknown option: '%c'\n", optopt);
				exit(3);
		}
	}

	if (idx.n) {
		fprintf(stderr, "Info: k = %u, w = %u, b = %u, m = %u & l = %u\n", k, w,
		        b, m, l);
		if (optind >= argc) {
			fputs("Error: expected reads file\n", stderr);
			exit(3);
		}

		FILE *read_fp = fopen(argv[optind], "r");
		if (read_fp == NULL) {
			fprintf(stderr, "Error: cannot open `%s`\n", argv[optind]);
			exit(1);
		}

		read_v reads;
		parse_fastq(read_fp, &reads);
		fprintf(stderr, "Info: Reads file: %s read\n", argv[optind]);

		if (target.n) {
			compare(target, reads, idx, READ_LENGTH, w, k, b, m, l);
		} else {
			location_v *locs =
			    (location_v *)malloc(sizeof(location_v) * reads.n);
			if (locs == NULL) {
				fputs("Memory error\n", stderr);
				exit(2);
			}
			for (size_t i = 0; i < reads.n; i++) {
				cseeding(idx, reads.a[i], READ_LENGTH, w, k, b, m, l, &locs[i]);
				for (size_t j = 0; j < locs[i].n; j++) {
					if (j) {
						printf("\t");
					}
					printf("%u", locs[i].a[j]);
				}
				puts("");
			}
		}

	} else {

		if (optind + 1 >= argc) {
			fputs("Error: expected genome reference file name & output file "
			      "name\n",
			      stderr);
			exit(3);
		}

		FILE *in_fp = fopen(argv[optind], "r");
		if (in_fp == NULL) {
			fprintf(stderr, "Error: cannot open `%s`\n", argv[optind]);
			exit(1);
		}

		FILE *out_fp = fopen(argv[optind + 1], "wb");
		if (out_fp == NULL) {
			fprintf(stderr, "Error: cannot open `%s`\n", argv[optind + 1]);
			exit(1);
		}

		printf("Info: w = %u, k = %u, f = %u & b = %u\n", w, k, f, b);
		if (r) {
			puts("Info: Output format: sorted array of (minimizer, location, "
			     "strand)");
			mm72_v r_idx;
			create_raw_index(in_fp, w, k, f, b, &r_idx);
			uint8_t *a = (uint8_t *)malloc(sizeof(uint8_t) * r_idx.n * 9);
			if (a == NULL) {
				fputs("Memory error\n", stderr);
				exit(2);
			}
			uint32_t mask = (1 << 8) - 1;
			for (size_t i = 0; i < r_idx.n; i++) {
				a[9 * i]     = (uint8_t)r_idx.a[i].minimizer & mask;
				a[9 * i + 1] = (uint8_t)(r_idx.a[i].minimizer >> 8) & mask;
				a[9 * i + 2] = (uint8_t)(r_idx.a[i].minimizer >> 16) & mask;
				a[9 * i + 3] = (uint8_t)(r_idx.a[i].minimizer >> 24) & mask;
				a[9 * i + 4] = (uint8_t)r_idx.a[i].location & mask;
				a[9 * i + 5] = (uint8_t)(r_idx.a[i].location >> 8) & mask;
				a[9 * i + 6] = (uint8_t)(r_idx.a[i].location >> 16) & mask;
				a[9 * i + 7] = (uint8_t)(r_idx.a[i].location >> 24) & mask;
				a[9 * i + 8] = (uint8_t)r_idx.a[i].strand;
			}
			fwrite(&(r_idx.n), sizeof(r_idx.n), 1, out_fp);
			fwrite(a, sizeof(uint8_t), r_idx.n * 9, out_fp);
			fclose(out_fp);
			printf("Info: Binary file `%s` written\n", argv[optind + 1]);
		} else {
			if (compact) {
				puts("Info: Output format: minimizer array, location array");
				create_cindex(in_fp, w, k, f, b, &idx);
				fwrite(&(idx.n), sizeof(idx.n), 1, out_fp);
				fwrite(idx.h, sizeof(idx.h[0]), idx.n, out_fp);
				fwrite(idx.location, sizeof(idx.location[0]), idx.m, out_fp);
				fclose(out_fp);
				printf("Info: Binary file `%s` written\n", argv[optind + 1]);
			} else {
				puts("Info: Output format: minimizer array, location array, "
				     "strand "
				     "array");
				index_t idx = {.n        = 0,
				               .m        = 0,
				               .h        = NULL,
				               .location = NULL,
				               .strand   = NULL};
				create_index(in_fp, w, k, f, b, &idx);
				fwrite(&(idx.n), sizeof(idx.n), 1, out_fp);
				fwrite(idx.h, sizeof(idx.h[0]), idx.n, out_fp);
				fwrite(idx.location, sizeof(idx.location[0]), idx.m, out_fp);
				fwrite(idx.strand, sizeof(idx.strand[0]), idx.m, out_fp);
				fclose(out_fp);
				printf("Info: Binary file `%s` written\n", argv[optind + 1]);

				if (p) {
					FILE *gnuplot = popen("gnuplot", "w");
					fprintf(gnuplot, "set terminal png size 1200, 900\n");
					// fprintf(gnuplot, "set logscale y\n");
					fprintf(gnuplot, "set output 'cumulative.png'\n");
					fprintf(
					    gnuplot,
					    "set title 'Cumulative sum of the number of entries "
					    "(k = %u, w = %u, f = %u and b = %u)'\n",
					    k, w, f, b);
					fprintf(gnuplot, "set xlabel 'Minimizers'\n");
					fprintf(gnuplot, "set ylabel 'Cumulative sum of the number "
					                 "of locations'\n");
					fprintf(gnuplot, "plot '-' with lines lw 3 notitle\n");
					for (uint32_t i = 0; i < idx.n; i += idx.n / 1000) {
						fprintf(gnuplot, "%u %u\n", i, idx.h[i]);
					}
					fprintf(gnuplot, "e\n");
					fflush(gnuplot);
					pclose(gnuplot);
					puts("Info: cumulative.png written");
				}
			}
		}
	}
	return 0;
}
