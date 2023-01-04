#include "demeter_parsing.h"
#include "demeter_util.h"
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_SEQ_LEN 1 << 30
//#define ENCODE(c) (c & 0x0f) >> 1
#define END_OF_READ_BASE 'E'
#define IDX_MAGIC "ALOHA"

extern unsigned IDX_MAP_SIZE;
extern unsigned IDX_MAX_OCC;
extern unsigned SE_W;
extern unsigned SE_K;

static int fastq_fd;
static uint8_t *fastq_file_ptr;
static size_t fastq_file_len;
static size_t fastq_file_pos;
static uint8_t *fastq_buf;

void fastq_open(int param, const char *file_name) {
	struct stat statbuf;
	fastq_fd = open(file_name, O_RDONLY);
	if (fastq_fd == -1) {
		err(1, "open %s", file_name);
	}
	if (fstat(fastq_fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	fastq_file_len = statbuf.st_size;
	if (param == OPEN_MMAP) {
		fastq_file_ptr = (uint8_t *)mmap(NULL, fastq_file_len, PROT_READ,
		                                 MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK, fastq_fd, 0);
		if (fastq_file_ptr == MAP_FAILED) {
			err(1, "%s:%d, mmap", __FILE__, __LINE__);
		}
	} else {
		// With Malloc and copy
		FILE *fp;
		FOPEN(fp, file_name, "rb");
		MALLOC(fastq_file_ptr, uint8_t, fastq_file_len);
		FREAD(fastq_file_ptr, uint8_t, fastq_file_len, fp);
	}
	fastq_file_pos = 0;
	MALLOC(fastq_buf, uint8_t, MAX_SEQ_LEN);
}

void read_buf_init(read_buf_t *const buf, const uint32_t capacity) {
	buf->capacity = capacity;
	buf->len      = 0;
	POSIX_MEMALIGN(buf->seq, 4096, capacity);
	// MALLOC(buf->seq, uint8_t, capacity);
	buf->metadata_len      = 0;
	buf->metadata_capacity = 0;
	buf->metadata          = NULL;
}

void read_buf_destroy(const read_buf_t buf) {
	free(buf.seq);
	free(buf.metadata);
}

int fastq_parse(read_buf_t *const buf) {
	static uint32_t cur_batch_id = 0;
	buf->len                     = 0;
	buf->metadata_len            = 0;
	buf->batch_id                = cur_batch_id;
	cur_batch_id++;

	while (fastq_file_pos < fastq_file_len) {
		// Get the name
		const size_t name_pos = fastq_file_pos + 1;
		uint8_t c             = fastq_file_ptr[name_pos];
		unsigned name_len     = 0;

		while (c != ' ' && c != '\n') {
			name_len++;
			c = fastq_file_ptr[name_pos + name_len];
		}

		size_t cur_pos = name_len + name_pos;
		name_len++;

		for (; fastq_file_ptr[cur_pos] != '\n'; cur_pos++) {
		}
		cur_pos++;

		// Get the sequence in buf
		uint32_t read_len = 0;
		for (;;) {
			uint8_t base_c = fastq_file_ptr[cur_pos + read_len];
			if (base_c == '\n') {
				break;
			}
			fastq_buf[read_len] = base_c;
			read_len++;
		}
		read_len++;

		// Check if it fits into the buf
		if (buf->len + read_len > buf->capacity) {
			return 0;
		}

		// Copy the seq into the buf
		fastq_buf[read_len - 1] = END_OF_READ_BASE;
		memcpy(&buf->seq[buf->len], fastq_buf, sizeof(uint8_t) * read_len);
		buf->len += read_len;

		// Increase the size of name buffer if necessary
		if (buf->metadata_len >= buf->metadata_capacity) {
			buf->metadata_capacity = buf->metadata_capacity ? 2 * buf->metadata_capacity : 1;
			REALLOC(buf->metadata, read_metadata_t, buf->metadata_capacity);
		}

		// Set the metadata
		// Alloc & copy the name into the buf
		MALLOC(buf->metadata[buf->metadata_len].name, char, name_len);
		memcpy(buf->metadata[buf->metadata_len].name, &fastq_file_ptr[name_pos], name_len);
		buf->metadata[buf->metadata_len].name[name_len - 1] = '\0';
		buf->metadata[buf->metadata_len].len                = read_len - 1;

		buf->metadata_len++;

		// Increment the pos
		fastq_file_pos = cur_pos + 2 * read_len + 2;
	}
	return 1;
}

void fastq_close() {
	munmap(fastq_file_ptr, fastq_file_len);
	free(fastq_buf);
	close(fastq_fd);
}

index_t index_parse(const char *const file_name) {
	FILE *fp;
	FOPEN(fp, file_name, "rb");
	char magic[5];
	if (fread(magic, sizeof(char), 5, fp) != 5) {
		errx(1, "parse_index %s", file_name);
	}
	if (strncmp(magic, IDX_MAGIC, 5) != 0) {
		errx(1, "parse_index %s", file_name);
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
	if (IDX_MAP_SIZE != param) {
		IDX_MAP_SIZE = param;
		fprintf(stderr, "[WARNING] parameter MAP_SIZE overriden by the index parameter (%u)\n", IDX_MAP_SIZE);
	}
	FREAD(&param, unsigned, 1, fp);
	if (IDX_MAX_OCC != param) {
		IDX_MAX_OCC = param;
		fprintf(stderr, "[WARNING] parameter IDX_MAX_OCC overriden by the index parameter (%u)\n", IDX_MAX_OCC);
	}
	index_t index;
	FREAD(&index.key_len, uint32_t, 1, fp);
	POSIX_MEMALIGN(index.map, 4096, (1ULL << IDX_MAP_SIZE) * sizeof(uint32_t));
	POSIX_MEMALIGN(index.key, 4096, index.key_len * sizeof(uint64_t));
	FREAD(index.map, uint32_t, 1ULL << IDX_MAP_SIZE, fp);
	FREAD(index.key, uint64_t, index.key_len, fp);
	FREAD(&index.nb_seq, uint32_t, 1, fp);
	MALLOC(index.seq_name, char *, index.nb_seq);
	MALLOC(index.seq_len, uint32_t, index.nb_seq);
	for (unsigned i = 0; i < index.nb_seq; i++) {
		uint8_t len;
		FREAD(&len, uint8_t, 1, fp);
		MALLOC(index.seq_name[i], char, len + 1);
		FREAD(index.seq_name[i], char, len, fp);
		index.seq_name[i][len] = '\0';
		FREAD(&index.seq_len[i], uint32_t, 1, fp);
	}
	return index;
}

void index_destroy(const index_t index) {
	free(index.map);
	free(index.key);
}
