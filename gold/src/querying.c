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
			// printf("start: %x, end: %x, bucket_id: %x\n", start, end, bucket_id);
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

#define LOC_SHIFT 42

void query_index_key(const pos_v pos_i, const uint64_t *key_i, loc_v *const location_o) {
	loc_v buf1 = {.capacity = 1 << 10, .len = 0};
	MALLOC(buf1.locs, loc_t, buf1.capacity);

	loc_v buf2 = {.capacity = 1 << 10, .len = 0};
	MALLOC(buf2.locs, loc_t, buf2.capacity);

	loc_v *buf_i = &buf1;
	loc_v *buf_o = &buf2;

	for (uint32_t i = 0; i < pos_i.len; i++) {
		pos_t pos                = pos_i.poss[i];
		const uint32_t start_pos = pos.start_pos;
		const uint32_t end_pos   = pos.end_pos;

		uint32_t new_start = start_pos;
		for (uint32_t j = start_pos; j < end_pos; j++) {
			const uint64_t key = key_i[j];
			uint32_t seed_id   = key >> IDX_B;
			if (seed_id == pos.seed_id) {
				break;
			}
			new_start++;
		}
		if (new_start != end_pos) {
			uint32_t buf_j     = 0;
			uint32_t key_j     = new_start;
			const uint64_t key = key_i[key_j];
			uint32_t seed_id   = key >> IDX_B;

			unsigned str      = ((key >> LOC_SHIFT) & 1) ^ pos.str;
			uint16_t chrom_id = (key >> 32) & ((1 << 10) - 1);
			// TODO: loc, depending on the strand
			uint32_t loc = key & ((1ULL << 32) - 1);
			while (buf_j < buf_i->len && key_j < end_pos && seed_id == pos.seed_id) {
				// First compare the str, then the chrom_id then the loc
				// TODO: compare
			}
		}
	}
}
