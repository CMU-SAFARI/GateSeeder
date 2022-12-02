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

			// std::cout << std::hex << "start: " << start << " end: " << end << std::endl;

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

ap_uint<1> query_index_key_MS(hls::stream<ms_pos_t> &ms_pos_i, const uint64_t *key_i, uint64_t *buf) {
	ms_pos_t pos     = ms_pos_i.read();
	uint32_t buf_len = 0;

	while (pos.EOR != 1) {
		std::cout << "start_pos: " << std::hex << pos.start_pos << " end_pos: " << pos.end_pos
		          << " seed_id: " << pos.seed_id << std::dec << " query_loc: " << pos.query_loc << std::endl;
		// Eliminate the false-positive
		const uint32_t start_pos = pos.start_pos.to_uint();
		const uint32_t end_pos   = pos.end_pos.to_uint();

		uint32_t new_start = start_pos;

		for (uint32_t i = start_pos; i < end_pos; i++) {
			ap_uint<64> key               = ap_uint<64>(key_i[i]);
			ap_uint<seed_id_size> seed_id = key.range(LOC_SHIFT + seed_id_size, seed_id_size);
			if (seed_id == pos.seed_id) {
				break;
			}
			new_start++;
		}

		if (new_start != end_pos) {
			std::cout << "Some position" << std::endl;
			// Merge
			uint32_t buf_j = 0;
			uint32_t key_j = 0;

			ap_uint<1> key_end = 0;
			while (key_end == 0 && buf_j < buf_len) {
				// TODO: both strand
			}
		}

		pos = ms_pos_i.read();
		/*
		std::cout << "EOR: " << pos.EOR << " start_pos: " << std::hex << pos.start_pos
		          << " end_pos: " << pos.end_pos << " seed_id: " << pos.seed_id << std::dec
		          << " query_loc: " << pos.query_loc << std::endl;
		          */
	}

	if (pos.str == 1) {
		return 1;
	} else {
		return 0;
	}
}

void query_index_key(hls::stream<ms_pos_t> &ms_pos_0_i, hls::stream<ms_pos_t> &ms_pos_1_i, const uint64_t *key_0_i,
                     const uint64_t *key_1_i, uint64_t *buf_0_i, uint64_t *buf_1_i, hls::stream<loc_t> &location_o) {
	ap_uint<1> res0 = query_index_key_MS(ms_pos_0_i, key_0_i, buf_0_i);
	ap_uint<1> res1 = query_index_key_MS(ms_pos_1_i, key_1_i, buf_1_i);

	while (!res0 && !res1) {
		// merge
		// TODO: merge both strand

		res0 = query_index_key_MS(ms_pos_0_i, key_0_i, buf_0_i);
		res1 = query_index_key_MS(ms_pos_1_i, key_1_i, buf_1_i);
	}
	location_o << loc_t{.target_loc = 0, .query_loc = 0, .chrom_id = 0, .str = 1, .EOR = 1};
}
