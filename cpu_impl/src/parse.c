#include "parse.h"
#include "seeding.h"
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

void parse_index(int fd, index_t *idx) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	off_t size        = statbuf.st_size;
	uint32_t *idx_buf = (uint32_t *)mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (idx_buf == MAP_FAILED) {
		err(1, "mmap");
	}
	idx->n        = idx_buf[0];
	idx->h        = &idx_buf[1];
	idx->m        = idx->h[idx->n - 1];
	idx->location = &idx_buf[idx->n + 1];

	// Check the consistency of the data
	if ((((size_t)idx->m + idx->n + 1) << 2) != size) {
		errx(1, "parse_index");
	}
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
#ifdef VARIABLE_LEN
	reads->n   = 0;
	reads->a   = NULL;
	reads->len = NULL;
	size_t len = 0;
	char *init = read_buf;
	for (size_t i = 0; i < size; i++) {
		if ((read_buf[i] & 0xf) == 0xf) {
			reads->n++;
			reads->a = (char **)realloc(reads->a, reads->n * sizeof(char *));
			if (reads->a == NULL) {
				err(1, "realloc");
			}
			reads->a[reads->n - 1] = init;
			init                   = &read_buf[i + 1];
			reads->len             = (size_t *)realloc(reads->len, reads->n * sizeof(size_t));
			if (reads->len == NULL) {
				err(1, "realloc");
			}
			reads->len[reads->n - 1] = len;
			len                      = 0;
		} else if (((read_buf[i] >> 4) & 0xf) == 0xf) {
			reads->n++;
			reads->a = (char **)realloc(reads->a, reads->n * sizeof(char *));
			if (reads->a == NULL) {
				err(1, "realloc");
			}
			reads->a[reads->n - 1] = init;
			init                   = &read_buf[i + 1];
			reads->len             = (size_t *)realloc(reads->len, reads->n * sizeof(size_t));
			if (reads->len == NULL) {
				err(1, "realloc");
			}
			reads->len[reads->n - 1] = len + 1;
			len                      = 0;
		} else {
			len += 2;
		}
	}
#else
	reads->n = 2 * size / READ_LEN;
	reads->a = (char **)malloc(sizeof(char *) * reads->n);
	if (reads->a == NULL) {
		err(1, "malloc");
	}
	for (size_t i = 0; i < reads->n; i++) {
		reads->a[i] = &read_buf[i * (READ_LEN / 2 + READ_LEN % 2)];
	}
#endif
}
