#include "querying.hpp"

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_0_o,
                     hls::stream<ms_pos_t> &ms_pos_1_o) {
	seed_t seed = seed_i.read();

	while (seed.str == 0 || seed.EOR == 0) {
		if (seed.EOR == 1) {
			ms_pos_0_o << ms_pos_t{.str = 0, .EOR = 1};
			ms_pos_1_o << ms_pos_t{.str = 0, .EOR = 1};
		} else {
			const uint32_t bucket_id = seed.hash.range(bucket_id_msb, 0).to_uint();
			// access the map
			const ap_uint<32> start = ap_uint<32>((bucket_id == 0) ? 0 : map_i[bucket_id - 1]);
			const ap_uint<32> end   = ap_uint<32>(map_i[bucket_id]);

			// std::cout << std::hex << "start: " << start << std::endl;

			// If there are some locations
			if (start != end) {

				// find out in which MS are the locations
				const ap_uint<ms_id_size> ms_id_start = start.range(31, ms_id_lsb);
				const ap_uint<ms_id_size> ms_id       = end.range(31, ms_id_lsb);

				ms_pos_t ms_pos;
				ms_pos.end_pos   = end.range(ms_pos_msb, 0);
				ms_pos.seed_id   = seed.hash.range(seed_id_msb, seed_id_lsb);
				ms_pos.query_loc = seed.loc.to_uint();
				ms_pos.str       = seed.str;
				ms_pos.EOR       = 0;

				// In case we are at the begining of a MS
				if (ms_id != ms_id_start) {
					ms_pos.start_pos = 0;
				} else {
					ms_pos.start_pos = start.range(ms_pos_msb, 0);
				}

				switch (ms_id) {
					case 0:
						ms_pos_0_o << ms_pos;
						break;
					default:
						ms_pos_1_o << ms_pos;
						break;
				}
			}
		}
		seed = seed_i.read();
	}
	ms_pos_0_o << ms_pos_t{.str = 1, .EOR = 1};
	ms_pos_1_o << ms_pos_t{.str = 1, .EOR = 1};
}

void query_index_key(hls::stream<ms_pos_t> &ms_pos_0_i, hls::stream<ms_pos_t> &ms_pos_1_i, const uint64_t *key_0_i,
                     const uint64_t *key_1_i, hls::stream<loc_t> &location_o) {
	// TODO
}
