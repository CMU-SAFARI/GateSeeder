#include "extraction.hpp"
#include "kernel.hpp"
#include "querying.hpp"
#include "types.hpp"
#include <stdint.h>

void kernel(const uint32_t nb_bases_i, const uint8_t *seq_i, const uint32_t *map_i, const uint64_t *key_0_i,
            const uint64_t *key_1_i, uint64_t *buf_0_i, uint64_t *buf_1_i, const uint64_t *out_o) {

#pragma HLS INTERFACE m_axi port = seq_i bundle = gmem0
#pragma HLS INTERFACE m_axi port = out_o bundle = gmem0

#pragma HLS INTERFACE m_axi port = map_i bundle = gmem1
#pragma HLS INTERFACE m_axi port = key_0_i bundle = gmem2
#pragma HLS INTERFACE m_axi port = key_1_i bundle = gmem3

#pragma HLS INTERFACE s_axilite port = seq_i
#pragma HLS INTERFACE s_axilite port = out_o

#pragma HLS INTERFACE s_axilite port = map_i
#pragma HLS INTERFACE s_axilite port = key_0_i
#pragma HLS INTERFACE s_axilite port = key_1_i

#pragma HLS dataflow

	hls::stream<seed_t> seed;

	hls::stream<ms_pos_t> ms_pos_0;
	hls::stream<ms_pos_t> ms_pos_1;

	hls::stream<loc_t> location;

	extract_seeds(seq_i, nb_bases_i, seed);

#ifdef DEBUG_SEED_EXTRACTION
	while (!seed.empty()) {
		seed_t s = seed.read();
		std::cout << "hash: " << std::hex << s.hash << " loc: " << std::dec << s.loc << std::endl;
	}
#else
	query_index_map(seed, map_i, ms_pos_0, ms_pos_1);

#ifdef DEBUG_QUERY_INDEX_MAP
	std::cout << "MS_0:" << std::endl;
	while (!ms_pos_0.empty()) {
		ms_pos_t p = ms_pos_0.read();
		std::cout << "start_pos: " << std::hex << p.start_pos << " end_pos: " << p.end_pos
		          << " seed_id: " << p.seed_id << " loc: " << p.query_loc << std::endl;
	}
	std::cout << "MS_1:" << std::endl;
	while (!ms_pos_1.empty()) {
		ms_pos_t p = ms_pos_1.read();
		std::cout << "start_pos: " << std::hex << p.start_pos << " end_pos: " << p.end_pos
		          << " seed_id: " << p.seed_id << " loc: " << p.query_loc << std::endl;
	}

#else
	query_index_key(ms_pos_0, ms_pos_1, key_0_i, key_1_i, buf_0_i, buf_1_i, location);
#ifdef DEBUG_QUERY_INDEX_KEY
#endif
#endif
#endif

	// vote();
}
