#ifndef QUERYING_H
#define QUERYING_H

#include "extraction.h"

typedef struct {
	uint32_t start_pos;
	uint32_t end_pos;
	uint32_t seed_id;
	uint32_t query_loc;
	uint8_t str;
} pos_t;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	pos_t *poss;
} pos_v;

void query_index_map(const seed_v seed_i, const uint32_t *const map_i, pos_v *const pos_o);

#endif
