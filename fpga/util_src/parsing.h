#ifndef PARSING_H
#define PARSING_H

#ifdef __cplusplus
extern "C" {
#endif
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
	unsigned nb_MS;
	uint32_t *map;
	uint64_t **key;
} index_t;

void open_fastq(const char *const file_name);
void read_buf_init(read_buf_t *const buf, const uint32_t capacity);
int parse_fastq(read_buf_t *const buf);
void close_fastq();
index_t parse_index(const char *const file_name);

#ifdef __cplusplus
}
#endif
#endif
