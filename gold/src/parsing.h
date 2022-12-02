#ifndef PARSING_DEBUG_H
#define PARSING_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdio.h>

typedef struct {
	uint32_t id;
	uint32_t len;
	char *name;
	uint8_t *seq;
	char *qual;
} read_t;

typedef struct {
	unsigned nb_reads;
	read_t *reads;
} read_v;

typedef struct {
	uint32_t map_len, key_len;
	uint32_t *map;
	uint64_t *key;
} index_t;

void open_reads(int fd);
void parse_reads(read_v *input);
index_t parse_index(const char *const file_name);

#ifdef __cplusplus
}
#endif
#endif
