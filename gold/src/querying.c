#include "querying.h"
#include "util.h"

extern unsigned IDX_B;
#define BUCKET_MASK ((1 << IDX_B) - 1)

inline void push_pos(const pos_t pos_i, pos_v *const pos_o) {
	if (pos_o->capacity == pos_o->len) {
		pos_o->capacity <<= 1;
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
			const uint32_t seed_id = seed.hash >> IDX_B;
			pos_t pos              = {.start_pos = start,
			                          .end_pos   = end,
			                          .seed_id   = seed_id,
			                          .query_loc = seed.loc,
			                          .str       = seed.str};
			push_pos(pos, pos_o);
		}
	}
}
