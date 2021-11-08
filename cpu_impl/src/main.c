#include "seeding.h"
#include <stdlib.h>
#include <unistd.h>
#define BUFFER_SIZE 4294967296

typedef struct {
	size_t n;
	uint32_t *loc;
} loc_v;

typedef struct {
	size_t n;
	loc_v *loc;
} exp_loc_v;

void parse_data(FILE *fp, exp_loc_v *loc);

int main(int argc, char *argv[]) {
	int option;
	exp_loc_v loc;
	while ((option = getopt(argc, argv, "t:")) != -1) {
		switch (option) {
			case 't': {
				FILE *fp = fopen(optarg, "r");
				if (fp == NULL) {
					fprintf(stderr, "Error: cannot open `%s`\n", optarg);
					exit(1);
				}
				parse_data(fp, &loc);
				for (size_t i = 0; i < loc.n; i++) {
					for (size_t j = 0; j < loc.loc[i].n; j++) {
						printf("%u\n", loc.loc[i].loc[j]);
					}
				}
				break;
			}
		}
	}
	return 0;
}

void parse_data(FILE *fp, exp_loc_v *loc) {
	char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	if (buffer == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}

	fread(buffer, sizeof(char), BUFFER_SIZE, fp);
	if (!feof(fp)) {
		fputs("Reading error: buffer too small\n", stderr);
		fclose(fp);
		exit(3);
	}
	loc->n               = 0;
	loc->loc             = NULL;
	uint32_t *loc_buffer = NULL;
	size_t loc_buffer_n  = 0;
	char num[11]         = {0};
	size_t num_i         = 0;
	size_t i             = 0;
	for (;;) {
		char c = buffer[i];
		switch (c) {
			case '\n':
				if (num_i != 0) {
					num[num_i] = '\0';
					num_i      = 0;
					loc_buffer_n++;
					loc_buffer = (uint32_t *)realloc(loc_buffer, loc_buffer_n * sizeof(uint32_t));
					if (loc_buffer == NULL) {
						fputs("Memory error\n", stderr);
						exit(2);
					}
					loc_buffer[loc_buffer_n - 1] = strtoul(num, NULL, 10);
				}
				loc->n++;
				loc->loc = (loc_v *)realloc(loc->loc, loc->n * sizeof(loc_v));
				if (loc->loc == NULL) {
					fputs("Memory error\n", stderr);
					exit(2);
				}
				loc->loc[loc->n - 1] = (loc_v){loc_buffer_n, loc_buffer};
				loc_buffer           = NULL;
				loc_buffer_n         = 0;
				break;
			case '\t':
				num[num_i] = '\0';
				num_i      = 0;
				loc_buffer_n++;
				loc_buffer = (uint32_t *)realloc(loc_buffer, loc_buffer_n * sizeof(uint32_t));
				if (loc_buffer == NULL) {
					fputs("Memory error\n", stderr);
					exit(2);
				}
				loc_buffer[loc_buffer_n - 1] = strtoul(num, NULL, 10);
				break;
			case 0:
				free(buffer);
				return;
			default:
				num[num_i] = c;
				num_i++;
				break;
		}
		i++;
	}
}
