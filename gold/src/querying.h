#ifndef QUERYING_H
#define QUERYING_H

#include "extraction.h"

typedef struct {
	uint32_t start_pos;
	uint32_t end_pos;
	uint32_t seed_id;
	uint32_t query_loc;
	unsigned str : 1;
} pos_t;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	pos_t *poss;
} pos_v;

typedef struct {
	uint32_t target_loc;
	uint32_t query_loc;
	uint16_t chrom_id;
	unsigned str : 1;
} loc_t;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	loc_t *locs;
} loc_v;

void query_index_map(const seed_v seed_i, const uint32_t *const map_i, pos_v *const pos_o);

void query_index_key(const pos_v pos_i, const uint64_t *key_i, loc_v *const location_o);
#endif
