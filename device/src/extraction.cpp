#include "extraction.hpp"

static const ap_uint<64> HASH_MAX = (1UL << 2 * SE_K) - 1;

#define END_OF_READ_BASE 'E'
#define N_BASE 'N'

const unsigned window_size = SE_W;
typedef ap_uint<8> base_t;

ap_uint<64> hash64(ap_uint<64> key) {
	key = (~key + (key << 21)) & HASH_MAX; // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)) & HASH_MAX; // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)) & HASH_MAX; // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31)) & HASH_MAX;
	return key;
}

void extract_seeds(const uint8_t *seq_i, const uint32_t nb_bases_i, hls::stream<seed_t> &minimizers_o) {
	seed_t previous_minimizer             = {.hash = HASH_MAX, .loc = 0, .str = 0, .EOR = 1};
	ap_uint<READ_SIZE> location           = 0;
	ap_uint<READ_SIZE> length             = 0;
	ap_uint<2 *SE_K + 2 *SE_W - 2> window = 0, window_rv = 0;

seed_extraction_loop:
	for (uint32_t base_n = 0; base_n < nb_bases_i; base_n++) {
#pragma HLS PIPELINE II = 1
		ap_uint<1> EOR;
		seed_t min;

		ap_uint<2 * SE_K> kmers[SE_W][2];
		seed_t base_window[SE_W];
#pragma HLS array_partition type = complete variable = base_window

		// Read
		base_t base(seq_i[base_n]);

		if (base == END_OF_READ_BASE) {
			EOR      = 1;
			location = 0;
			length   = 0;
			min      = {.hash = HASH_MAX, .loc = 0, .str = 0, .EOR = 1};
		} else {
			EOR = 0;
			location++;
			if (base == N_BASE) {
				length = 0;
			} else {
				length++;
				window <<= 2;
				// Encode
				window(1, 0) = base(2, 1);
				window_rv >>= 2;
				window_rv(2 * SE_W + 2 * SE_K - 3, 2 * SE_W + 2 * SE_K - 4) = (base ^ 0x4)(2, 1);
			}
		kmer_gen_loop:
			for (unsigned i = 0; i < SE_W; i++) {
#pragma HLS UNROLL
				kmers[i][0] = window(2 * SE_W - 2 * i + 2 * SE_K - 3, 2 * SE_W - 2 * i - 2) & HASH_MAX;
				kmers[i][1] = window_rv(2 * SE_K + 2 * i - 1, 2 * i);
			}
		hash_loop:
			for (unsigned i = 0; i < SE_W; i++) {
#pragma HLS UNROLL
				if (kmers[i][0] == kmers[i][1]) {
					base_window[i].hash = HASH_MAX;
				} else {
					ap_uint<1> z        = kmers[i][0] > kmers[i][1];
					base_window[i].hash = hash64(kmers[i][z]);
					base_window[i].str  = z;
					base_window[i].loc  = location + i - SE_W;
					base_window[i].EOR  = 0;
				}
				// DEBUG
				/*
				std::cout << std::hex << "hash : " << base_window[i].hash
				          << " loc: " << base_window[i].loc << std::endl;
				          */
			}

			// Find the minimum
			GET_MIN(min, base_window);
		}

		// Write
		if (EOR || (length > SE_K + SE_W - 2 && min.loc != previous_minimizer.loc)) {
			minimizers_o << min;
			previous_minimizer = min;
		}
	}
	minimizers_o << seed_t{.hash = HASH_MAX, .loc = 0, .str = 1, .EOR = 1};
}
