#include "extraction.hpp"
#include "kernel.hpp"
#include "querying.hpp"
#include "types.hpp"
#include <stdint.h>

void demeter_kernel(const uint32_t nb_bases_i, const uint8_t seq_i[SEQ_LEN], uint64_t out_o[OUT_LEN],
                    const uint32_t map_i[MAP_LEN], const uint64_t key_0_i[KEY_LEN], const uint64_t key_1_i[KEY_LEN]) {

#pragma HLS INTERFACE m_axi port = seq_i bundle = gmem0
#pragma HLS INTERFACE m_axi port = map_i bundle = gmem1
#pragma HLS INTERFACE m_axi port = key_0_i bundle = gmem2
#pragma HLS INTERFACE m_axi port = key_1_i bundle = gmem3
#pragma HLS INTERFACE m_axi port = out_o bundle = gmem4

#pragma HLS INTERFACE s_axilite port = seq_i
#pragma HLS INTERFACE s_axilite port = out_o

#pragma HLS INTERFACE s_axilite port = map_i
#pragma HLS INTERFACE s_axilite port = key_0_i
#pragma HLS INTERFACE s_axilite port = key_1_i

#pragma HLS dataflow

	hls::stream<seed_t, 1024> seed;

	hls::stream<ms_pos_t, 1024> ms_pos;

	hls::stream<uint64_t, 1024> loc;

	extract_seeds(seq_i, nb_bases_i, seed);

#ifdef DEBUG_SEED_EXTRACTION
	unsigned counter = 0;
	while (!seed.empty()) {
		seed_t s = seed.read();
		std::cout << "hash: " << std::hex << s.hash << " loc: " << s.loc << std::endl;
		if (s.EOR == 0) {
			counter++;
		}
	}
	std::cout << "Nb_seeds: " << std::dec << counter << std::endl;
#else
	query_index_map(seed, map_i, ms_pos);

#ifdef DEBUG_QUERY_INDEX_MAP
	std::cout << "MAP:" << std::endl;
	while (!ms_pos.empty()) {
		ms_pos_t p = ms_pos.read();
		std::cout << "start_pos: " << std::hex << p.start_pos << " end_pos: " << p.end_pos
		          << " ms_id: " << p.ms_id << " seed_id: " << p.seed_id << " loc: " << p.query_loc << std::endl;
	}
#else

	query_index_key(ms_pos, key_0_i, key_1_i, loc);

#ifdef DEBUG_QUERY_INDEX_KEY
	unsigned counter = 0;
	while (!loc.empty()) {
		uint64_t l = loc.read();
		std::cout << std::hex << l << std::endl;
		if (l != (1ULL << 63) && l != UINT64_MAX) {
			counter++;
		}
	}
	std::cout << "Nb_locs: " << std::dec << counter << std::endl;
#else
	write_loc(loc, out_o);
#endif
#endif
#endif
}
