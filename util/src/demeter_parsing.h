#ifndef DEMETER_PARSING_H
#define DEMETER_PARSING_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
	char *name;
	uint32_t len;

} read_metadata_t;

typedef struct {
	uint32_t batch_id;
	uint32_t capacity; // in terms of bps
	uint32_t len;      // in terms of bps
	uint8_t *seq;
	uint32_t metadata_len;
	uint32_t metadata_capacity;
	read_metadata_t *metadata;
} read_buf_t;

typedef struct {
	uint32_t w;
	uint32_t k;
	uint32_t map_size;
	uint32_t max_occ;
	uint32_t key_len;
	uint32_t *map;
	uint64_t *key;
	uint32_t nb_seq;
	char **seq_name;
	uint32_t *seq_len;
} index_t;

#define OPEN_MALLOC 0
#define OPEN_MMAP 1

void fa_open(const int param, const char *const file_name);
void read_buf_init(read_buf_t *const buf, const uint32_t capacity);
void read_buf_destroy(const read_buf_t buf);
int fa_parse(read_buf_t *const buf);
void fa_close();
index_t index_parse(const char *const file_name);
void index_destroy_key_map(const index_t index);
void index_destroy_target(const index_t index);

#endif
