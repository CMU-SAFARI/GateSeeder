#ifndef DEMETER_PARSING_H
#define DEMETER_PARSING_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
	uint32_t capacity; // in terms of bps
	uint32_t len;      // in terms of bps
	uint8_t *seq;
	unsigned nb_seqs;
	unsigned seq_name_capacity;
	char **seq_name;
} read_buf_t;

typedef struct {
	uint32_t key_len;
	uint32_t *map;
	uint64_t *key;
} index_t;

#define OPEN_MALLOC 0
#define OPEN_MMAP 1

void open_fastq(int param, const char *const file_name);
void read_buf_init(read_buf_t *const buf, const uint32_t capacity);
void read_buf_destroy(const read_buf_t buf);
int parse_fastq(read_buf_t *const buf);
void close_fastq();
index_t parse_index(const char *const file_name);
void index_destroy(const index_t index);

#endif
