#include "parse.h"
#include "seeding.h"
#include "stdlib.h"
#define BUFFER_SIZE 4294967296
#include <assert.h>
#include <string.h>

void parse_dat(FILE *fp, exp_loc_v *loc) {
	char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	if (buffer == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}

	fread(buffer, sizeof(char), BUFFER_SIZE, fp);
	if (!feof(fp)) {
		fputs("Reading error: buffer too small\n", stderr);
		exit(3);
	}
	fclose(fp);
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

void parse_index(FILE *fp, index_t *idx) {
	fread(&idx->n, sizeof(uint32_t), 1, fp);

	idx->h = (uint32_t *)malloc(sizeof(uint32_t) * idx->n);
	if (idx->h == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}
	fread(idx->h, sizeof(uint32_t), idx->n, fp);
	idx->m = idx->h[idx->n - 1];

	idx->location = (uint32_t *)malloc(sizeof(uint32_t) * idx->m);
	if (idx->location == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}
	fread(idx->location, sizeof(uint32_t), idx->m, fp);

	// Check if we reached the EOF
	uint8_t eof;
	fread(&eof, sizeof(uint8_t), 1, fp);
	if (!feof(fp)) {
		fputs("Reading error: wrong file format\n", stderr);
		exit(3);
	}

	fclose(fp);
	return;
}

void parse_fastq(FILE *fp, read_v *reads) {
	char *read_buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	if (read_buffer == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}

	fread(read_buffer, sizeof(char), BUFFER_SIZE, fp);
	if (!feof(fp)) {
		fputs("Reading error: buffer too small\n", stderr);
		exit(3);
	}
	fclose(fp);

	reads->n    = 0;
	reads->a    = NULL;
	reads->name = NULL;

	size_t i              = 0;
	char name_buffer[100] = {0};
	unsigned char name_i  = 0;

	for (;;) {
		char c = read_buffer[i];

		if (c == '@') {
			i++;
			c      = read_buffer[i];
			name_i = 0;
			while (c != ' ') {
				name_buffer[name_i] = c;
				name_i++;
				i++;
				c = read_buffer[i];
			}
			name_buffer[name_i] = '\0';

			while (c != '\n') {
				i++;
				c = read_buffer[i];
			}
			reads->n++;
			reads->a = (char **)realloc(reads->a, reads->n * sizeof(char *));
			if (reads->a == NULL) {
				fputs("Memory error\n", stderr);
				exit(2);
			}

			reads->name = (char **)realloc(reads->name, reads->n * sizeof(char *));
			if (reads->name == NULL) {
				fputs("Memory error\n", stderr);
				exit(2);
			}

			reads->a[reads->n - 1] = (char *)malloc(READ_LEN * sizeof(char));
			if (reads->a[reads->n - 1] == NULL) {
				fputs("Memory error\n", stderr);
				exit(2);
			}
			for (size_t j = 0; j < READ_LEN; j++) {
				i++;
				reads->a[reads->n - 1][j] = read_buffer[i];
			}

			reads->name[reads->n - 1] = (char *)malloc(100 * sizeof(char));
			if (reads->name[reads->n - 1] == NULL) {
				fputs("Memory error\n", stderr);
				exit(2);
			}
			strcpy(reads->name[reads->n - 1], name_buffer);
			i++;
			assert(read_buffer[i] == '\n');
			i++;
			assert(read_buffer[i] == '+');
			i = READ_LEN + i;
		} else if (c == 0) {
			break;
		}
		i++;
	}
	free(read_buffer);
	return;
}
