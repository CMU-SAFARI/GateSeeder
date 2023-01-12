#include "demeter_parsing.h"
#include "demeter_util.h"
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_SEQ_LEN 1 << 30
#define END_OF_READ_BASE 'E'
#define IDX_MAGIC "ALOHA"

static int fastq_fd;
static uint8_t *fastq_file_ptr;
static size_t fastq_file_len;
static size_t fastq_file_pos;
static uint8_t *fastq_buf;

void fastq_open(const int param, const char *const file_name) {
	struct stat statbuf;
	OPEN(fastq_fd, file_name, O_RDONLY);
	if (fstat(fastq_fd, &statbuf) == -1) {
		err(1, "%s:%d, fstat", __FILE__, __LINE__);
	}
	fastq_file_len = statbuf.st_size;
	if (param == OPEN_MMAP) {
		MMAP(fastq_file_ptr, uint8_t, fastq_file_len, PROT_READ, MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK,
		     fastq_fd);
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
		int is_fasta          = fastq_file_ptr[fastq_file_pos] == '>';
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
		fastq_file_pos = cur_pos + read_len;
		if (!is_fasta) {
			while (fastq_file_ptr[fastq_file_pos] != '\n') {
				fastq_file_pos++;
			}
			fastq_file_pos += read_len + 1;
		}
	}
	return 1;
}

void fastq_close() {
	MUNMAP(fastq_file_ptr, fastq_file_len);
	free(fastq_buf);
	CLOSE(fastq_fd);
}

index_t index_parse(const char *const file_name) {
	FILE *fp;
	FOPEN(fp, file_name, "rb");
	char magic[5];
	FREAD(magic, char, 5, fp);
	if (strncmp(magic, IDX_MAGIC, 5) != 0) {
		errx(1, "%s:%d, parse_index %s", __FILE__, __LINE__, file_name);
	}

	index_t index;
	FREAD(&index.w, uint32_t, 1, fp);
	FREAD(&index.k, uint32_t, 1, fp);
	FREAD(&index.map_size, uint32_t, 1, fp);
	FREAD(&index.max_occ, uint32_t, 1, fp);
	FREAD(&index.key_len, uint32_t, 1, fp);
	POSIX_MEMALIGN(index.map, 4096, (1ULL << index.map_size) * sizeof(uint32_t));
	POSIX_MEMALIGN(index.key, 4096, index.key_len * sizeof(uint64_t));
	FREAD(index.map, uint32_t, 1ULL << index.map_size, fp);
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
	FCLOSE(fp);
	return index;
}

void index_destroy_key_map(const index_t index) {
	free(index.map);
	free(index.key);
}

void index_destroy_target(const index_t index) {
	free(index.seq_len);
	free(index.seq_name);
}
