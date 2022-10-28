#include "parsing.h"
#include "util.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

extern target_t TARGET;
extern index_t INDEX;
extern unsigned BATCH_SIZE;
extern unsigned IDX_B;
extern unsigned IDX_MAX_OCC;
extern unsigned SE_K;
extern unsigned SE_W;

#define ENCODE(c) (c & 0x0f) >> 1
#define REVERSE(c) c ^ 0x02

#define MAX_SEQ_LEN 1 << 30

void parse_target(int fd, const int rev_enable) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	off_t size        = statbuf.st_size;
	uint8_t *file_buf = (uint8_t *)mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (file_buf == MAP_FAILED) {
		err(1, "%s:%d, mmap", __FILE__, __LINE__);
	}

	uint8_t *seq_buf;
	MALLOC(seq_buf, uint8_t, MAX_SEQ_LEN);

	TARGET.nb_sequences = 0;
	TARGET.len          = NULL;
	TARGET.seq          = NULL;
	TARGET.seq_reverse  = NULL;
	TARGET.name         = NULL;

	uint32_t seq_len = 0;
	int first_seq    = 1;

	for (size_t i = 0; i < (size_t)size; i++) {
		uint8_t c = file_buf[i];
		if (c == '>') {
			if (first_seq) {
				first_seq = 0;
			} else {
				TARGET.len[TARGET.nb_sequences - 1] = seq_len;
				MALLOC(TARGET.seq[TARGET.nb_sequences - 1], uint8_t, seq_len + 7);
				uint8_t *seq = TARGET.seq[TARGET.nb_sequences - 1];
				if (rev_enable) {
					REALLOC(TARGET.seq_reverse, uint8_t *, TARGET.nb_sequences);
					MALLOC(TARGET.seq_reverse[TARGET.nb_sequences - 1], uint8_t, seq_len + 7);
					uint8_t *seq_reverse = TARGET.seq_reverse[TARGET.nb_sequences - 1];
					for (uint32_t j = 0; j < seq_len; j++) {
						seq[j]                       = seq_buf[j];
						seq_reverse[seq_len - j - 1] = REVERSE(seq_buf[j]);
					}
					seq_reverse[seq_len] = '\0';
				} else {
					for (uint32_t j = 0; j < seq_len; j++) {
						seq[j] = seq_buf[j];
					}
				}
				seq[seq_len] = '\0';
			}
			TARGET.nb_sequences++;
			seq_len = 0;
			REALLOC(TARGET.name, char *, TARGET.nb_sequences);
			REALLOC(TARGET.len, uint32_t, TARGET.nb_sequences);
			REALLOC(TARGET.seq, uint8_t *, TARGET.nb_sequences);
			i++;
			c                 = file_buf[i];
			unsigned name_len = 0;
			while (c != ' ' && c != '\n') {
				name_len++;
				c = file_buf[i + name_len];
			}
			MALLOC(TARGET.name[TARGET.nb_sequences - 1], char, name_len + 1);
			memccpy(TARGET.name[TARGET.nb_sequences - 1], &file_buf[i], 0, name_len);
			TARGET.name[TARGET.nb_sequences - 1][name_len] = '\0';
			i += name_len;
			for (; file_buf[i] != '\n'; i++) {
			}
		} else {
			while (c != '\n') {
				seq_buf[seq_len] = ENCODE(c);
				seq_len++;
				i++;
				c = file_buf[i];
			}
		}
	}
	TARGET.len[TARGET.nb_sequences - 1] = seq_len;
	MALLOC(TARGET.seq[TARGET.nb_sequences - 1], uint8_t, seq_len + 7);
	uint8_t *seq = TARGET.seq[TARGET.nb_sequences - 1];
	if (rev_enable) {
		REALLOC(TARGET.seq_reverse, uint8_t *, TARGET.nb_sequences);
		MALLOC(TARGET.seq_reverse[TARGET.nb_sequences - 1], uint8_t, seq_len + 7);
		uint8_t *seq_reverse = TARGET.seq_reverse[TARGET.nb_sequences - 1];
		for (uint32_t j = 0; j < seq_len; j++) {
			seq[j]                       = seq_buf[j];
			seq_reverse[seq_len - j - 1] = REVERSE(seq_buf[j]);
		}
		seq_reverse[seq_len] = '\0';
	} else {
		for (uint32_t j = 0; j < seq_len; j++) {
			seq[j] = seq_buf[j];
		}
	}
	seq[seq_len] = '\0';
	munmap(file_buf, size);
	free(seq_buf);
}

void target_destroy() {
	for (unsigned i = 0; i < TARGET.nb_sequences; i++) {
		free(TARGET.name[i]);
		free(TARGET.seq[i]);
		if (TARGET.seq_reverse != NULL) {
			free(TARGET.seq_reverse[i]);
		}
	}
	free(TARGET.len);
	free(TARGET.seq);
	free(TARGET.seq_reverse);
	free(TARGET.name);
}

int parse_index(FILE *fp) {
	char magic[4];
	if (fread(magic, sizeof(char), 4, fp) != 4) {
		return 1;
	}
	if (strncmp(magic, IDX_MAGIC, 4) != 0) {
		return 1;
	}
	unsigned param;
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
	FREAD(&INDEX.key_len, uint32_t, 1, fp);
	MALLOC(INDEX.key, key128_t, INDEX.key_len);
	FREAD(&INDEX.map_len, uint32_t, 1, fp);
	MALLOC(INDEX.map, uint32_t, INDEX.map_len);
	FREAD(INDEX.key, key128_t, INDEX.key_len, fp);
	FREAD(INDEX.map, uint32_t, INDEX.map_len, fp);
	FREAD(&TARGET.nb_sequences, uint32_t, 1, fp);
	MALLOC(TARGET.len, uint32_t, TARGET.nb_sequences);
	MALLOC(TARGET.seq, uint8_t *, TARGET.nb_sequences);
	MALLOC(TARGET.seq_reverse, uint8_t *, TARGET.nb_sequences);
	MALLOC(TARGET.name, char *, TARGET.nb_sequences);
	for (unsigned i = 0; i < TARGET.nb_sequences; i++) {
		uint8_t len;
		FREAD(&len, uint8_t, 1, fp);
		MALLOC(TARGET.name[i], char, len + 1);
		FREAD(TARGET.name[i], char, len, fp);
		TARGET.name[i][len] = '\0';
		FREAD(&TARGET.len[i], uint32_t, 1, fp);
		MALLOC(TARGET.seq[i], uint8_t, TARGET.len[i]);
		MALLOC(TARGET.seq_reverse[i], uint8_t, TARGET.len[i]);
		FREAD(TARGET.seq[i], uint8_t, TARGET.len[i], fp);
		for (uint32_t j = 0; j < TARGET.len[i]; j++) {
			TARGET.seq_reverse[i][TARGET.len[i] - j - 1] = REVERSE(TARGET.seq[i][j]);
		}
	}
	return 0;
}

void parse_fastq(int fd, uint8_t *seq) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	off_t file_len = statbuf.st_size;
	uint8_t *read_file_ptr =
	    (uint8_t *)mmap(NULL, read_file_len, PROT_READ, MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK, fd, 0);

	if (read_file_ptr == MAP_FAILED) {
		err(1, "%s:%d, mmap", __FILE__, __LINE__);
	}

	for (size_t i = 0; i < (size_t)file_len; i++) {
		size_t j          = 1;
		uint8_t c         = read_file_ptr[j];
		unsigned name_len = 0;
		while (c != ' ' && c != '\n') {
			name_len++;
			c = read_file_ptr[j + name_len];
		}
		/*
		MALLOC(reads[i].name, char, name_len + 1);
		memccpy(reads[i].name, &read_file_ptr[j], 0, name_len);
		reads[i].name[name_len] = '\0';
		reads[i].id             = id;
		id++;
		j += name_len;
		*/
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
		// +7 because when we pack the read for the avx implemntation of seed extraction
		MALLOC(reads[i].seq, uint8_t, read_len + 7);
		MALLOC(reads[i].qual, char, read_len + 1);
		memcpy(reads[i].seq, buf, sizeof(uint8_t) * read_len);
		reads[i].seq[read_len]  = '\0';
		reads[i].qual[read_len] = '\0';
		reads[i].len            = read_len;
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
