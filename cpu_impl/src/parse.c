#include "parse.h"
#include "seeding.h"
#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

void parse_dat(int fd, exp_loc_v *loc) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	off_t size   = statbuf.st_size;
	char *buffer = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (buffer == MAP_FAILED) {
		err(1, "mmap");
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
				munmap(buffer, size);
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
	size_t ret = fread(&idx->n, sizeof(uint32_t), 1, fp);
	if (ret != 1) {
		fprintf(stderr, "fread() failed: %zu\n", ret);
		exit(EXIT_FAILURE);
	}

	idx->h = (uint32_t *)malloc(sizeof(uint32_t) * idx->n);
	if (idx->h == NULL) {
		err(1, "malloc");
	}
	ret = fread(idx->h, sizeof(uint32_t), idx->n, fp);
	if (ret != idx->n) {
		fprintf(stderr, "fread() failed: %zu\n", ret);
		exit(EXIT_FAILURE);
	}
	idx->m = idx->h[idx->n - 1];

	idx->location = (uint32_t *)malloc(sizeof(uint32_t) * idx->m);
	if (idx->location == NULL) {
		err(1, "malloc");
	}
	ret = fread(idx->location, sizeof(uint32_t), idx->m, fp);
	if (ret != idx->m) {
		fprintf(stderr, "fread() failed: %zu\n", ret);
		exit(EXIT_FAILURE);
	}

	// Check if we reached the EOF
	uint8_t eof;
	ret = fread(&eof, sizeof(uint8_t), 1, fp);
	if (ret != 0 || !feof(fp)) {
		fprintf(stderr, "fread() failed: %zu\n", ret);
		exit(EXIT_FAILURE);
	}

	fclose(fp);
	return;
}

void parse_reads(int fd, read_v *reads) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	off_t size     = statbuf.st_size;
	char *read_buf = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (read_buf == MAP_FAILED) {
		err(1, "mmap");
	}

	reads->n = 2 * size / READ_LEN;
	reads->a = (char **)malloc(sizeof(char **) * reads->n);
	for (size_t i = 0; i < reads->n; i++) {
		reads->a[i] = &read_buf[i * (READ_LEN / 2 + READ_LEN % 2)];
	}
}
