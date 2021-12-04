#include "encoding.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

static inline FILE *open_file(char *name, char *suffix) {
	char *name_buf = (char *)malloc(strlen(name) + strlen(suffix) + 1);
	if (name_buf == NULL) {
		err(1, "malloc");
	}
	stpcpy(name_buf, name);
	strcat(name_buf, suffix);
	FILE *fp_out = fopen(name_buf, "wb");
	if (fp_out == NULL) {
		err(1, "fopen %s", name_buf);
	}
	free(name_buf);
	return fp_out;
}

static unsigned char seq_nt4_table[256] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

void encode_const_len(int fd, unsigned l, char *name) {
	struct stat stat_buf;
	if (fstat(fd, &stat_buf) == -1) {
		err(1, "fstat");
	}
	off_t size     = stat_buf.st_size;
	char *read_buf = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (read_buf == MAP_FAILED) {
		err(1, "mmap");
	}
	FILE *fp_out = open_file(name, ".bin");
	size_t i     = 0;

	char *write_buf = (char *)malloc(l / 2 + l % 2);
	if (write_buf == NULL) {
		err(1, "malloc");
	}

	while (i < size) {
		while (read_buf[i] != '\n') {
			i++;
		}
		i++;
		for (size_t j = 0; j < l / 2; j++) {
			write_buf[j] =
			    seq_nt4_table[(size_t)read_buf[i]] | (seq_nt4_table[(size_t)read_buf[i + 1]] << 4);
			i += 2;
		}
		if (l % 2) {
			write_buf[l / 2] = seq_nt4_table[(size_t)read_buf[i]];
			i++;
		}
		fwrite(write_buf, 1, l / 2 + l % 2, fp_out);
		i++;
		while (read_buf[i] != '\n') {
			i++;
		}
		i += (l + 2);
	}
	munmap(read_buf, size);
}

void encode_var_len(int fd, char *name) {}
