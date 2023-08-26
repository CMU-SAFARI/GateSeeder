#include "GateSeeder_util.h"
#include "querying.h"

extern uint32_t IDX_MAP_SIZE;
extern uint32_t SE_K;
#define BUCKET_MASK ((1 << IDX_MAP_SIZE) - 1)
#define LOC_SHIFT 42
#define LOC_OFFSET (1 << 21)

inline void push_loc(const uint64_t location, location_v *const locations) {
	if (locations->capacity == locations->len) {
		locations->capacity = (locations->capacity == 0) ? 1 : locations->capacity << 1;
		REALLOC(locations->a, uint64_t, locations->capacity);
	}
	locations->a[locations->len] = location;
	locations->len++;
}

inline uint64_t key_2_loc(const uint64_t key, const unsigned query_str, const uint32_t query_loc) {
	// Extract the target_loc, the chrom_id, the str and the seed_id from the key
	const uint64_t key_target_loc = key & UINT32_MAX;
	const uint64_t chrom_id       = (key >> 32) & ((1 << 10) - 1);
	const uint64_t str            = ((key >> LOC_SHIFT) & 1) ^ query_str;

	// Compute the shifted target_loc
	const uint64_t target_loc = key_target_loc + (str ? query_loc : LOC_OFFSET - query_loc);

	// Pack
	const uint64_t loc = (chrom_id << 53) | (target_loc << 23) | (query_loc << 1) | str;
	return loc;
}

void query_index(seed_v seeds, location_v *locations, const index_t index) {
	locations->len = 0;
	for (uint32_t i = 0; i < seeds.len; i++) {
		const seed_t seed = seeds.a[i];

		const uint32_t bucket_id = seed.hash & BUCKET_MASK;
		const uint32_t seed_id   = seed.hash >> IDX_MAP_SIZE;

		const uint32_t start = (bucket_id == 0) ? 0 : index.map[bucket_id - 1];
		const uint32_t end   = index.map[bucket_id];

		for (uint32_t i = start; i < end; i++) {
			const uint64_t key      = index.key[i];
			const uint32_t kseed_id = key >> (LOC_SHIFT + 1);
			if (kseed_id == seed_id) {
				const uint64_t loc = key_2_loc(key, seed.str, seed.loc);
				push_loc(loc, locations);
			}
		}
	}
}
