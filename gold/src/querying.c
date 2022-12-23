#include "demeter_util.h"
#include "querying.h"

extern unsigned IDX_MAP_SIZE;
extern unsigned SE_K;
#define BUCKET_MASK ((1 << IDX_MAP_SIZE) - 1)

inline void push_pos(const pos_t pos_i, pos_v *const pos_o) {
	if (pos_o->capacity == pos_o->len) {
		pos_o->capacity = (pos_o->capacity == 0) ? 1 : pos_o->capacity << 1;
		REALLOC(pos_o->poss, pos_t, pos_o->capacity);
	}
	pos_o->poss[pos_o->len] = pos_i;
	pos_o->len++;
}

void query_index_map(const seed_v seed_i, const uint32_t *const map_i, pos_v *const pos_o) {
	pos_o->len = 0;
	for (uint32_t i = 0; i < seed_i.len; i++) {
		const seed_t seed        = seed_i.seeds[i];
		const uint32_t bucket_id = seed.hash & BUCKET_MASK;
		const uint32_t start     = (bucket_id == 0) ? 0 : map_i[bucket_id - 1];
		const uint32_t end       = map_i[bucket_id];

		if (start != end) {
			// printf("start: %x, end: %x, bucket_id: %x\n", start, end, bucket_id);
			const uint32_t seed_id = seed.hash >> IDX_MAP_SIZE;
			pos_t pos              = {.start_pos = start,
			                          .end_pos   = end,
			                          .seed_id   = seed_id,
			                          .query_loc = seed.loc,
			                          .str       = seed.str};
			push_pos(pos, pos_o);
		}
	}
}

#define LOC_SHIFT 42
#define LOC_OFFSET (1 << 21)

inline loc_t key_2_loc(const uint64_t key_i, const unsigned query_str_i, const uint32_t query_loc_i) {
	loc_t loc;
	// Extract the target_loc, the chrom_id, the str and the seed_id from the key
	const uint32_t key_target_loc = key_i & UINT32_MAX;
	loc.chrom_id                  = (key_i >> 32) & ((1 << 10) - 1);
	loc.str                       = ((key_i >> LOC_SHIFT) & 1) ^ query_str_i;
	loc.query_loc                 = query_loc_i;

	// Compute the shifted target_loc
	if (loc.str) {
		loc.target_loc = key_target_loc + query_loc_i - SE_K + 1;
	} else {
		loc.target_loc = key_target_loc + LOC_OFFSET - query_loc_i;
	}
	/*
	printf("target_loc: %x\n", key_target_loc);
	printf("target_loc: %x\n", key_target_loc + query_loc_i);
	printf("target_loc: %x\n", key_target_loc + query_loc_i - SE_K + 1);
	printf("target_loc_last: %x\n", loc.target_loc);
	*/

	return loc;
}

inline void print_loc(loc_t loc) {
	printf("target_loc: 0x%x, query_loc: 0x%x, chrom_id: 0x%x, str: 0x%x\n", loc.target_loc, loc.query_loc,
	       loc.chrom_id, loc.str);
}

inline void push_loc(const loc_t location_i, loc_v *const location_o) {
	if (location_o->capacity == location_o->len) {
		location_o->capacity = (location_o->capacity == 0) ? 1 : location_o->capacity << 1;
		REALLOC(location_o->locs, loc_t, location_o->capacity);
	}
	location_o->locs[location_o->len] = location_i;
	location_o->len++;
}

void query_index_key(const pos_v pos_i, const uint64_t *key_i, loc_v *const location_o) {
	location_o->len = 0;
	// printf("len: %u\n", pos_i.len);
	for (uint32_t i = 0; i < pos_i.len; i++) {
		/*
		printf("start_pos: %x end_pod: %x seed_id: %x query_loc %x\n", start_pos, end_pos, pos.seed_id,
		       query_loc);

		       */
		const pos_t pos = pos_i.poss[i];
		for (uint32_t j = pos.start_pos; j < pos.end_pos; j++) {
			const uint64_t key     = key_i[j];
			const uint32_t seed_id = key >> (LOC_SHIFT + 1);

			if (seed_id == pos.seed_id) {
				const loc_t loc = key_2_loc(key, pos.str, pos.query_loc);
				push_loc(loc, location_o);
			}
		}
	}
}
