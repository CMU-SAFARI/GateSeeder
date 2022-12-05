#include "querying.h"
#include "util.h"

extern unsigned IDX_B;
extern unsigned SE_K;
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
#define LOC_OFFSET (1 << 21)

inline loc_t extract_key_loc(const uint64_t key_i, const unsigned query_str_i, const uint32_t query_loc_i) {
	loc_t loc;
	// Extract the target_loc, the chrom_id, the str and the seed_id from the key
	const uint32_t key_target_loc = key_i & UINT32_MAX;
	loc.chrom_id                  = (key_i >> 32) & ((1 << 10) - 1);
	loc.str                       = ((key_i >> LOC_SHIFT) & 1) ^ query_str_i;
	loc.query_loc                 = query_loc_i;

	// Compute the shifted target_loc
	if (query_str_i) {
		loc.target_loc = key_target_loc + query_loc_i - SE_K + 1;
	} else {
		loc.target_loc = key_target_loc + LOC_OFFSET - query_loc_i;
	}

	return loc;
}

void query_index_key(const pos_v pos_i, const uint64_t *key_i, loc_v *const location_o) {
	loc_v buf1 = {.capacity = 1 << 24, .len = 0};
	// TODO: free
	MALLOC(buf1.locs, loc_t, buf1.capacity);

	loc_v buf2 = {.capacity = 1 << 24, .len = 0};
	// TODO: free
	MALLOC(buf2.locs, loc_t, buf2.capacity);

	loc_v *buf_i = &buf1;
	loc_v *buf_o = &buf2;

	for (uint32_t i = 0; i < pos_i.len; i++) {
		const pos_t pos          = pos_i.poss[i];
		const uint32_t start_pos = pos.start_pos;
		const uint32_t end_pos   = pos.end_pos;
		const uint32_t query_loc = pos.query_loc;
		const unsigned query_str = pos.str;

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
			buf_o->len = 0;

			uint32_t key_j     = new_start;
			const uint64_t key = key_i[key_j];

			loc_t loc_key    = extract_key_loc(key, query_str, query_loc);
			uint32_t seed_id = key >> IDX_B;

			// First case: need to merge
			if (buf_i->len != 0) {
				uint32_t buf_j = 0;

				loc_t loc_buf = buf_i->locs[buf_j];

				int merge = 1;
				while (merge) {
					int sel_buf = 0;
					// First compare the str, then the chrom_id then the target_loc
					if (loc_key.str > loc_buf.str) {
						sel_buf = 1;
					} else if (loc_key.str == loc_buf.str) {
						if (loc_key.chrom_id > loc_buf.chrom_id) {
							sel_buf = 1;
						} else if (loc_key.chrom_id == loc_buf.chrom_id) {
							if (loc_key.target_loc > loc_buf.target_loc) {
								sel_buf = 1;
							}
						}
					}

					if (sel_buf) {
						buf_o->locs[buf_o->len] = loc_buf;
						buf_j++;
						if (buf_j == buf_i->len) {
							merge = 0;
						} else {
							loc_buf = buf_i->locs[buf_j];
						}
					} else {
						buf_o->locs[buf_o->len] = loc_key;
						key_j++;
						if (key_j == end_pos) {
							merge = 0;
						} else {
							const uint64_t key = key_i[key_j];
							loc_key            = extract_key_loc(key, query_str, query_loc);
							seed_id            = key >> IDX_B;
							if (seed_id != pos.seed_id) {
								merge = 0;
							}
						}
					}
					buf_o->len++;
				}

				// Copy the last values
				if (buf_j != buf_i->len) {
					for (; buf_j < buf_i->len; buf_j++) {
						buf_o->locs[buf_o->len] = buf_i->locs[buf_j];
						buf_j++;
						buf_o->len++;
					}
				} else {
					int copy = 1;
					while (copy) {
						buf_o->locs[buf_o->len] = loc_key;
						key_j++;
						if (key_j == end_pos) {
							copy = 0;
						} else {
							const uint64_t key = key_i[key_j];
							loc_key            = extract_key_loc(key, query_str, query_loc);
							seed_id            = key >> IDX_B;
							if (seed_id != pos.seed_id) {
								copy = 0;
							}
						}
					}
				}
			}

			// Second case: we just need to copy
			else {
				int copy = 1;
				while (copy) {
					buf_o->locs[buf_o->len] = loc_key;
					key_j++;
					if (key_j == end_pos) {
						copy = 0;
					} else {
						const uint64_t key = key_i[key_j];
						loc_key            = extract_key_loc(key, query_str, pos.query_loc);
						seed_id            = key >> IDX_B;
						if (seed_id != pos.seed_id) {
							copy = 0;
						}
					}
				}
			}

			// Swap the buffers
			loc_v *tmp_buf = buf_o;
			buf_o          = buf_i;
			buf_i          = tmp_buf;
		}
	}
	*location_o = *buf_i;
	free(buf_o->locs);
}
