#include "parsing.h"
#include "util.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define IDX_MAGIC "ALOHA"

extern unsigned SE_W;
extern unsigned SE_K;
extern unsigned IDX_B;
extern unsigned IDX_MAX_OCC;

static uint8_t *read_file_ptr;
static size_t read_file_len;
extern unsigned BATCH_SIZE;
#define ENCODE(c) (c & 0x0f) >> 1

void open_reads(int fd) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	read_file_len = statbuf.st_size;
	read_file_ptr =
	    (uint8_t *)mmap(NULL, read_file_len, PROT_READ, MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK, fd, 0);
	if (read_file_ptr == MAP_FAILED) {
		err(1, "%s:%d, mmap", __FILE__, __LINE__);
	}
}

void parse_reads(read_v *input) {
	input->nb_reads       = 0;
	read_t *reads         = input->reads;
	static uint8_t *buf   = NULL;
	static size_t buf_len = 0;
	static uint32_t id    = 0;
	if (!read_file_len) {
		return;
	}
	for (size_t i = 0; i < BATCH_SIZE; i++) {
		size_t j          = 1;
		uint8_t c         = read_file_ptr[j];
		unsigned name_len = 0;
		while (c != ' ' && c != '\n') {
			name_len++;
			c = read_file_ptr[j + name_len];
		}
		MALLOC(reads[i].name, char, name_len + 1);
		memccpy(reads[i].name, &read_file_ptr[j], 0, name_len);
		reads[i].name[name_len] = '\0';
		reads[i].id             = id;
		id++;
		j += name_len;
		for (; read_file_ptr[j] != '\n'; j++) {
		}
		j++;
		uint32_t read_len = 0;
		for (;;) {
			if (read_len >= buf_len) {
				buf_len = 2 * (read_len + 1);
				REALLOC(buf, uint8_t, buf_len);
			}
			uint8_t base_c = read_file_ptr[j + read_len];
			if (base_c == '\n') {
				break;
			}
			buf[read_len] = ENCODE(base_c);
			read_len++;
		}
		MALLOC(reads[i].seq, uint8_t, read_len + 1);
		MALLOC(reads[i].qual, char, read_len + 1);
		memcpy(reads[i].seq, buf, sizeof(uint8_t) * read_len);
		reads[i].seq[read_len] = '\0';
		reads[i].len           = read_len;
		input->nb_reads++;
		j += read_len + 1;
		for (; read_file_ptr[j] != '\n'; j++) {
		}
		memcpy(reads[i].qual, &read_file_ptr[j + 1], sizeof(char) * read_len);
		reads[i].qual[read_len] = '\0';
		j += read_len + 2;
		read_file_len -= j;
		if (read_file_len == 0) {
			break;
		}
		read_file_ptr = &read_file_ptr[j];
	}
}

index_t parse_index(const char *const file_name) {
	FILE *fp = fopen(file_name, "rb");
	if (fp == NULL) {
		err(1, "fopen %s", file_name);
	}
	char magic[5];
	if (fread(magic, sizeof(char), 5, fp) != 5) {
		errx(1, "parse_index %s", file_name);
	}
	if (strncmp(magic, IDX_MAGIC, 5) != 0) {
		errx(1, "parse_index %s", file_name);
	}

	unsigned param;
	// TODO: handle errors
	FREAD(&param, unsigned, 1, fp);
	if (SE_W != param) {
		SE_W = param;
		fprintf(stderr, "[WARNING] parameter W overriden by the index parameter (%u)\n", SE_W);
	}
	FREAD(&param, unsigned, 1, fp);
	if (SE_K != param) {
		SE_K = param;
		fprintf(stderr, "[WARNING] parameter K overriden by the index parameter (%u)\n", SE_K);
	}
	FREAD(&param, unsigned, 1, fp);
	if (IDX_B != param) {
		IDX_B = param;
		fprintf(stderr, "[WARNING] parameter B overriden by the index parameter (%u)\n", IDX_B);
	}
	FREAD(&param, unsigned, 1, fp);
	if (IDX_MAX_OCC != param) {
		IDX_MAX_OCC = param;
		fprintf(stderr, "[WARNING] parameter IDX_MAX_OCC overriden by the index parameter (%u)\n", IDX_MAX_OCC);
	}
	index_t index;
	FREAD(&index.key_len, uint32_t, 1, fp);
	// printf("key_len: %u\n", index.key_len);
	MALLOC(index.map, uint32_t, 1 << IDX_B);
	MALLOC(index.key, uint64_t, index.key_len);
	FREAD(index.map, uint32_t, 1ULL << IDX_B, fp);
	FREAD(index.key, uint64_t, index.key_len, fp);
	return index;
}
