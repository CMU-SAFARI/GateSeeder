#include "indexing.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	unsigned int w = 12;
	unsigned int k = 18;
	unsigned int f = 700;
	unsigned int b = 26;
	int option;
	while ((option = getopt(argc, argv, ":w:k:f:b:")) != -1) {
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
			case ':':
				fprintf(stderr, "Error: '%c' requires a value\n", optopt);
				exit(3);
			case '?':
				fprintf(stderr, "Error: unknown option: '%c'\n", optopt);
				exit(3);
		}
	}
}
