#include "extraction.hpp"

static const ap_uint<64> MASK = (1 << 2 * SE_K) - 1;
static const ap_uint<64> MAX  = (1 << 2 * SE_K) - 1;

#define END_OF_READ_BASE 'E'
#define N_BASE 'N'

const unsigned window_size = SE_W;
typedef ap_uint<8> base_t;

ap_uint<64> hash64(ap_uint<64> key) {
	key = (~key + (key << 21)) & MASK; // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)) & MASK; // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)) & MASK; // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31)) & MASK;
	return key;
}

void extract_seeds(const uint8_t *seq_i, const uint32_t nb_bases_i, hls::stream<seed_t> &minimizers_o) {
	seed_t previous_minimizer = {.hash = MAX, .loc = 0, .str = 0, .EOR = 1};
	ap_uint<32> location      = 0;
	ap_uint<32> length        = 0;
	ap_uint<2 * SE_K + 2 * SE_W - 2> window, window_rv;

seed_extraction_loop:
	for (uint32_t base_n = 0; base_n < nb_bases_i; base_n++) {
#pragma HLS PIPELINE II = window_size
		base_t base(seq_i[base_n]);
		ap_uint<1> EOR = 0, out[SE_W];
		ap_uint<2 * SE_K> kmers[SE_W][2];
		seed_t base_window[SE_W], base_window_2[SE_W];
#pragma HLS array_partition type = complete variable = base_window
#pragma HLS array_partition type = complete variable = base_window_2

		if (base == END_OF_READ_BASE) {
			EOR                = 1;
			length             = 0;
			location           = 0;
			previous_minimizer = {.hash = MAX, .loc = 0, .str = 0, .EOR = 1};
		} else {
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
				kmers[i][0] = window(2 * SE_W - 2 * i + 2 * SE_K - 3, 2 * SE_W - 2 * i - 2) & MASK;
				kmers[i][1] = window_rv(2 * SE_K + 2 * i - 1, 2 * i);
			}
		hash_loop:
			for (unsigned i = 0; i < SE_W; i++) {
#pragma HLS UNROLL
				if (kmers[i][0] == kmers[i][1]) {
					base_window[i].hash = MAX;
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

			// Copy to reduce the interval
		copy_loop:
			for (unsigned i = 0; i < SE_W; i++) {
#pragma HLS UNROLL
				base_window_2[i] = base_window[i];
			}

			uint64_t min = base_window[0].hash;
		comp_loop:
			for (unsigned i = 1; i < SE_W; i++) {
				if (base_window_2[i].hash < min) {
					min = base_window_2[i].hash;
				}
			}

			/*
			std::cout << "prev_minimum: hash: " << std::hex << previous_minimizer.hash
			          << " loc: " << previous_minimizer.loc << std::endl;
			          */
		find_minimizer_loop:
			for (unsigned i = 0; i < SE_W; i++) {
#pragma HLS UNROLL
				out[i] = (base_window_2[i].hash == min) ? 1 : 0;
				/*
				// DEBUG
				if (out[i] == 1) {
				        std::cout << "minimum: hash: " << std::hex << base_window_2[i].hash
				                  << " loc: " << base_window_2[i].loc << std::endl;
				}
				*/
			}
			EOR = 0;
		}

		// Write
		if (length > SE_K + SE_W - 2 || EOR) {
			if (EOR) {
				minimizers_o << previous_minimizer;
			} else {
				seed_t new_previous_minimizer = previous_minimizer;
				for (unsigned i = 0; i < SE_W; i++) {
					if (out[i] && base_window_2[i].loc > previous_minimizer.loc) {
						minimizers_o << base_window_2[i];
						new_previous_minimizer = base_window_2[i];
					}
				}
				previous_minimizer = new_previous_minimizer;
			}
			EOR = 0;
		}
	}
	minimizers_o << seed_t{.hash = MAX, .loc = 0, .str = 1, .EOR = 1};
}
