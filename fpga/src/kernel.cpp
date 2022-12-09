#include "extraction.hpp"
#include "kernel.hpp"
#include "querying.hpp"
#include "types.hpp"
#include <stdint.h>

void kernel(const uint32_t nb_bases_i, const uint8_t seq_i[SEQ_LEN], const uint32_t map_i[MAP_LEN],
            const uint64_t key_0_i[KEY_LEN], const uint64_t key_1_i[KEY_LEN], uint64_t buf_0_i[MS_BUF_LEN],
            uint64_t buf_1_i[MS_BUF_LEN], uint64_t out_o[OUT_LEN]) {

#pragma HLS INTERFACE m_axi port = seq_i bundle = gmem0
#pragma HLS INTERFACE m_axi port = out_o bundle = gmem1

#pragma HLS INTERFACE m_axi port = map_i bundle = gmem2
#pragma HLS INTERFACE m_axi port = key_0_i bundle = gmem3
#pragma HLS INTERFACE m_axi port = key_1_i bundle = gmem4
#pragma HLS INTERFACE m_axi port = buf_0_i bundle = gmem5
#pragma HLS INTERFACE m_axi port = buf_1_i bundle = gmem6

#pragma HLS INTERFACE s_axilite port = seq_i
#pragma HLS INTERFACE s_axilite port = out_o

#pragma HLS INTERFACE s_axilite port = map_i
#pragma HLS INTERFACE s_axilite port = key_0_i
#pragma HLS INTERFACE s_axilite port = key_1_i
#pragma HLS INTERFACE s_axilite port = buf_0_i
#pragma HLS INTERFACE s_axilite port = buf_1_i

#pragma HLS dataflow

	hls::stream<seed_t> seed;

	hls::stream<ms_pos_t> ms_pos_0;
	hls::stream<ms_pos_t> ms_pos_1;

	hls::stream<loc_t> location;

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
	unsigned counter = 0;
	while (!location.empty()) {
		loc_t loc = location.read();
		std::cout << std::hex << "target_loc: " << loc.target_loc << " query_loc: " << loc.query_loc
		          << " chrom_id: " << loc.chrom_id << std::endl;
		if (loc.EOR == 0) {
			counter++;
		}
	}
	std::cout << "Nb_locs: " << std::dec << counter << std::endl;
#else
	write_locations(location, out_o);
#endif
#endif
#endif
}
