#include "parsing.h"
#include "util.h"
#include <err.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_SEQ_LEN 1 << 30
#define ENCODE(c) (c & 0x0f) >> 1

target_t parse_target(int fd) {
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

	target_t target = {
	    .nb_sequences = 0,
	    .len          = NULL,
	    .seq          = NULL,
	    .name         = NULL,
	};

	uint32_t seq_len = 0;
	int first_seq    = 1;

	for (size_t i = 0; i < (size_t)size; i++) {
		uint8_t c = file_buf[i];
		if (c == '>') {
			if (first_seq) {
				first_seq = 0;
			} else {
				target.len[target.nb_sequences - 1] = seq_len;
				MALLOC(target.seq[target.nb_sequences - 1], uint8_t, seq_len + 7);
				uint8_t *seq = target.seq[target.nb_sequences - 1];
				for (uint32_t j = 0; j < seq_len; j++) {
					seq[j] = seq_buf[j];
				}
				seq[seq_len] = '\0';
			}
			target.nb_sequences++;
			seq_len = 0;
			REALLOC(target.name, char *, target.nb_sequences);
			REALLOC(target.len, uint32_t, target.nb_sequences);
			REALLOC(target.seq, uint8_t *, target.nb_sequences);
			i++;
			c                 = file_buf[i];
			unsigned name_len = 0;
			while (c != ' ' && c != '\n') {
				name_len++;
				c = file_buf[i + name_len];
			}
			MALLOC(target.name[target.nb_sequences - 1], char, name_len + 1);
			memccpy(target.name[target.nb_sequences - 1], &file_buf[i], 0, name_len);
			target.name[target.nb_sequences - 1][name_len] = '\0';
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
	target.len[target.nb_sequences - 1] = seq_len;
	MALLOC(target.seq[target.nb_sequences - 1], uint8_t, seq_len + 7);
	uint8_t *seq = target.seq[target.nb_sequences - 1];

	for (uint32_t j = 0; j < seq_len; j++) {
		seq[j] = seq_buf[j];
	}
	seq[seq_len] = '\0';
	munmap(file_buf, size);
	free(seq_buf);
	return target;
}
