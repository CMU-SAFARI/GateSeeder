#include "querying.hpp"
#include <string.h>

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_o) {
	seed_t seed = seed_i.read();

query_index_map_loop:
	while (seed.str == 0 || seed.EOR == 0) {
#pragma HLS pipeline II = 2
		if (seed.EOR == 1) {
			ms_pos_o << ms_pos_t{
			    .start_pos = 0, .end_pos = 0, .ms_id = 0, .seed_id = 0, .query_loc = 0, .str = 0, .EOR = 1};
		} else {
			const uint32_t bucket_id = seed.hash.range(bucket_id_msb, 0).to_uint();
			// access the map
			const ap_uint<32> start = ap_uint<32>((bucket_id == 0) ? 0 : map_i[bucket_id - 1]);
			const ap_uint<32> end   = ap_uint<32>(map_i[bucket_id]);

			// std::cout << std::hex << "start: " << start << " end: " << end << std::endl;

			// If there are some locations
			if (start != end) {

				ms_pos_t ms_pos;

				// find out in which MS are the locations
				const ap_uint<MS_ID_SIZE> ms_id_start = start.range(31, ms_id_lsb);
				ms_pos.ms_id                          = end.range(31, ms_id_lsb);

				ms_pos.end_pos   = end.range(ms_pos_msb, 0);
				ms_pos.seed_id   = seed.hash.range(seed_id_msb, seed_id_lsb);
				ms_pos.query_loc = seed.loc.to_uint();
				ms_pos.str       = seed.str;
				ms_pos.EOR       = 0;

				// In case we are at the begining of a MS
				if (ms_pos.ms_id != ms_id_start) {
					ms_pos.start_pos = 0;
				} else {
					ms_pos.start_pos = start.range(ms_pos_msb, 0);
				}

				ms_pos_o << ms_pos;
			}
		}
		seed = seed_i.read();
	}
	ms_pos_o << ms_pos_t{
	    .start_pos = 0, .end_pos = 0, .ms_id = 0, .seed_id = 0, .query_loc = 0, .str = 1, .EOR = 1};
}

#define LOC_OFFSET (1 << 21)
inline loc_t extract_key_loc(const uint64_t key_i, const ap_uint<1> query_str_i, const uint32_t query_loc_i) {
	ap_uint<64> key(key_i);
	// std::cout << "extract_key_loc: " << std::hex << key_i << std::endl;
	const ap_uint<1> str = key.range(42, 42) ^ query_str_i;
	ap_uint<30> target_loc =
	    key.range(29, 0) + (str ? ap_uint<30>(query_loc_i - SE_K + 1) : ap_uint<30>(LOC_OFFSET - query_loc_i));
	// std::cout << "target_loc: " << std::hex << target_loc << std::endl;
	return loc_t{
	    .target_loc = target_loc,
	    .query_loc  = ap_uint<21>(query_loc_i),
	    .chrom_id   = key.range(41, 32),
	    .str        = str,
	    .EOR        = ap_uint<1>(0),
	};
}

inline uint64_t loc_to_uint64(loc_t loc_i) {
	ap_uint<64> loc = (loc_i.EOR, loc_i.str, loc_i.chrom_id, loc_i.query_loc, loc_i.target_loc);
	return loc.to_uint64();
}

inline loc_t uint64_to_loc(uint64_t loc_i) {
	ap_uint<64> loc = ap_uint<64>(loc_i);
	return loc_t{
	    .target_loc = loc.range(29, 0),
	    .query_loc  = loc.range(50, 30),
	    .chrom_id   = loc.range(60, 51),
	    .str        = loc.range(62, 61),
	    .EOR        = loc.range(63, 63),
	};
}

void query_index_key_MS(const uint32_t start_pos_i, const uint32_t end_pos_i, const ap_uint<seed_id_size> seed_id_i,
                        const uint64_t *const key_i, hls::stream<uint64_t> &loc_o) {
	// Check if we find locations with the same seed_id
search_key_loop:
	for (uint32_t key_j = start_pos_i; key_j < end_pos_i; key_j++) {
		uint64_t key = key_i[key_j];

		const ap_uint<64> uint_key          = ap_uint<64>(key);
		const ap_uint<seed_id_size> seed_id = uint_key.range(LOC_SHIFT + 1 + seed_id_size, LOC_SHIFT + 1);

		if (seed_id == seed_id_i) {
			// Copy the locations
			loc_o << key;
		}
	}
}

void query_index_key(hls::stream<ms_pos_t> &ms_pos_i, const uint64_t *const key_0_i, const uint64_t *const key_1_i,
                     hls::stream<uint64_t> &loc_o) {
	ms_pos_t pos = ms_pos_i.read();

query_index_key_loop:
	while (pos.EOR == 0 || pos.str == 0) {
#pragma HLS loop_flatten off
		// EOR
		if (pos.EOR == 1) {
			loc_o << (1ULL << 63);
		} else {
			const uint32_t start_pos = pos.start_pos.to_uint();
			const uint32_t end_pos   = pos.end_pos.to_uint();

			switch (pos.ms_id) {
				case 0:
					query_index_key_MS(start_pos, end_pos, pos.seed_id, key_0_i, loc_o);
					break;
				default:
					query_index_key_MS(start_pos, end_pos, pos.seed_id, key_1_i, loc_o);
					break;
			}
		}
		pos = ms_pos_i.read();
	}
	loc_o << UINT64_MAX;
}

void write_loc(hls::stream<uint64_t> &loc_i, uint64_t *const loc_o) {
	uint64_t loc   = loc_i.read();
	uint32_t loc_j = 0;
write_loc_loop:
	while (loc != UINT64_MAX) {
#pragma HLS pipeline II = 2
		loc_o[loc_j] = loc;
		loc_j++;
		loc = loc_i.read();
	}
	loc_o[loc_j] = UINT64_MAX;
}
