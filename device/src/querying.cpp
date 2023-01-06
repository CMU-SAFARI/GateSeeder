#include "querying.hpp"
#include <string.h>

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_o) {
	seed_t seed = seed_i.read();

query_index_map_loop:
	while (seed.str == 0 || seed.EOR == 0) {
#pragma HLS pipeline II = 2
		if (seed.EOR == 1) {
			ms_pos_o << ms_pos_t{
			    .start_pos = 0, .end_pos = 0, /* TODO.seed_id = 0,*/ .query_loc = 0, .str = 0, .EOR = 1};
		} else {
			const uint32_t bucket_id = seed.hash.range(bucket_id_msb, 0).to_uint();
			// access the map
			const uint32_t start = (bucket_id == 0) ? 0 : map_i[bucket_id - 1];
			const uint32_t end   = map_i[bucket_id];

			// std::cout << std::hex << "start: " << start << " end: " << end << std::endl;

			// If there are some potential locations
			if (start != end) {
				ms_pos_o << ms_pos_t{.start_pos = start,
				                     .end_pos   = end,
				                     // TODO
				                     //.seed_id   = seed.hash(seed_id_msb, seed_id_lsb),
				                     .query_loc = seed.loc,
				                     .str       = seed.str,
				                     .EOR       = 0};
			}
		}
		seed = seed_i.read();
	}
	ms_pos_o << ms_pos_t{.start_pos = 0, .end_pos = 0, /* TODO.seed_id = 0,*/ .query_loc = 0, .str = 1, .EOR = 1};
}

inline uint64_t key_2_loc(const uint64_t key_i, const ap_uint<1> str_i, const ap_uint<READ_SIZE> query_loc_i) {
	const ap_uint<64> key                = ap_uint<64>(key_i);
	const ap_uint<CHROM_SIZE> target_loc = key.range(CHROM_SIZE - 1, 0);
	const ap_uint<1> str                 = str_i ^ key.range(KEY_STR_START, KEY_STR_START);
	const ap_uint<CHROM_SIZE> target_loc_shifted =
	    str ? ap_uint<CHROM_SIZE>(target_loc + query_loc_i)
	        : ap_uint<CHROM_SIZE>(target_loc + ap_uint<CHROM_SIZE>(LOC_OFFSET) - query_loc_i);
	const ap_uint<CHROM_ID_SIZE> chrom_id = key.range(CHROM_ID_SIZE - 1 + KEY_CHROM_ID_START, KEY_CHROM_ID_START);

	const ap_uint<64> loc = (ap_uint<1>(0), chrom_id, target_loc_shifted, query_loc_i, str);
	return loc.to_uint64();
}

void query_index_key(hls::stream<ms_pos_t> &ms_pos_i, const uint64_t *const key_i, hls::stream<uint64_t> &loc_o) {
	ms_pos_t pos = ms_pos_i.read();
query_index_key_loop:
	while (pos.EOR == 0 || pos.str == 0) {
#pragma HLS loop_flatten off
		// EOR
		if (pos.EOR == 1) {
			loc_o << (1ULL << 63);
		} else {
			// Check if we find locations with the same seed_id
		search_key_loop:
			for (uint32_t key_j = pos.start_pos; key_j < pos.end_pos; key_j++) {
				uint64_t key               = key_i[key_j];
				const ap_uint<64> uint_key = ap_uint<64>(key);
				// TODO
				/*const ap_uint<seed_id_size> seed_id =
				    uint_key.range(seed_id_size - 1 + KEY_SEED_ID_START, KEY_SEED_ID_START);
				    */
				// TODO
				/*if (seed_id == pos.seed_id) {*/
				// Copy the locations
				const uint64_t loc = key_2_loc(key, pos.str, pos.query_loc);
				loc_o << loc;
				//}
			}
		}
		pos = ms_pos_i.read();
	}
	loc_o << UINT64_MAX;
	loc_o << UINT64_MAX;
	loc_o << 0;
}

void write_loc(hls::stream<uint64_t> &loc_i, uint64_t *const loc_o) {
	uint64_t loc0  = loc_i.read();
	uint64_t loc1  = loc_i.read();
	uint32_t loc_j = 0;
write_loc_loop:
	while (loc0 != UINT64_MAX) {
#pragma HLS pipeline II = 2
		loc_o[loc_j]     = loc0;
		loc_o[loc_j + 1] = loc1;
		loc_j += 2;
		loc0 = loc_i.read();
		loc1 = loc_i.read();
	}

	if (loc1 == UINT64_MAX) {
		loc_o[loc_j] = UINT64_MAX;
		loc0         = loc_i.read();
	}
}
