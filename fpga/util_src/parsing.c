#include "parsing.h"
#include "util.h"
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

// TODO: maybe remove
#define MAX_SEQ_LEN 1 << 30
#define ENCODE(c) (c & 0x0f) >> 1

static uint8_t *fastq_file_ptr;
static size_t fastq_file_len;
static size_t fastq_file_pos;

void open_fastq(int fd) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	fastq_file_len = statbuf.st_size;
	fastq_file_ptr =
	    (uint8_t *)mmap(NULL, fastq_file_len, PROT_READ, MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK, fd, 0);
	if (fastq_file_ptr == MAP_FAILED) {
		err(1, "%s:%d, mmap", __FILE__, __LINE__);
	}
	fastq_file_pos = 0;
}

void read_buf_init(read_buf_t *const buf, const uint32_t capacity) {
	buf->capacity = capacity;
	buf->len      = 0;
	MALLOC(buf->seq, uint8_t, capacity);
	buf->nb_seqs           = 0;
	buf->seq_name_capacity = 0;
	buf->seq_name          = NULL;
}

void parse_fastq(int fd, read_buf_t *const buf) {
	uint32_t len     = 0;
	unsigned nb_seqs = 0;

	for (; fastq_file_pos < fastq_file_len; fastq_file_pos++) {
		const size_t name_pos = fastq_file_pos + 1;
		uint8_t c             = fastq_file_ptr[name_pos];
		unsigned name_len     = 0;

		while (c != ' ' && c != '\n') {
			name_len++;
			c = fastq_file_ptr[name_pos + name_len];
		}

		size_t cur_pos = name_len + name_pos;
		for (; fastq_file_ptr[cur_pos] != '\n'; cur_pos++) {
		}
		cur_pos++;

		uint32_t buf_pos = len;
		for (;;) {
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
		nb_seqs++;
		if (nb_seqs > buf->seq_name_capacity) {
		}
	}
}
