#include "extraction.hpp"
#include "kernel.hpp"
#include "querying.hpp"
#include "types.hpp"
#include <stdint.h>

void kernel(const uint32_t nb_bases_i, const uint8_t *seq_i, const uint32_t *map_i, const uint64_t *key_0_i,
            const uint64_t *key_1_i, const uint64_t *out_o) {

	// TODO: try with stream input and output

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

	extract_seeds(seq_i, nb_bases_i, seed);

	// DEBUG
	while (!seed.empty()) {
		seed_t s = seed.read();
		std::cout << "hash: " << s.hash << " loc: " << s.loc << std::endl;
	}
	// query_index_map(seed, map_i, ms_pos_0, ms_pos_1);

	// query_index_key(ms_pos_0,

	// vote();
}