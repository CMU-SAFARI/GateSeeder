#include "extraction.hpp"

static const ap_uint<64> MASK = (1UL << 2 * SE_K) - 1;
static const ap_uint<64> MAX  = (1UL << 2 * SE_K) - 1;

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
	seed_t previous_minimizer             = {.hash = MAX, .loc = 0, .str = 0, .EOR = 1};
	ap_uint<READ_SIZE> location           = 0;
	ap_uint<READ_SIZE> length             = 0;
	ap_uint<2 *SE_K + 2 *SE_W - 2> window = 0, window_rv = 0;

seed_extraction_loop:
	for (uint32_t base_n = 0; base_n < nb_bases_i; base_n++) {
#pragma HLS PIPELINE II = 1
		base_t base(seq_i[base_n]);
		ap_uint<1> EOR = 0;
		ap_uint<2 * SE_K> kmers[SE_W][2];
		seed_t base_window[SE_W];
#pragma HLS array_partition type = complete variable = base_window
		seed_t min;

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

			// Find the minimum
			const seed_t min_0 =
			    (base_window[0].hash <= base_window[1].hash) ? base_window[0] : base_window[1];
			const seed_t min_1 =
			    (base_window[2].hash <= base_window[3].hash) ? base_window[2] : base_window[3];
			const seed_t min_2 =
			    (base_window[4].hash <= base_window[5].hash) ? base_window[4] : base_window[5];
			const seed_t min_3 =
			    (base_window[6].hash <= base_window[7].hash) ? base_window[6] : base_window[7];
			const seed_t min_4 =
			    (base_window[8].hash <= base_window[9].hash) ? base_window[8] : base_window[9];
			const seed_t min_5 =
			    (base_window[10].hash <= base_window[11].hash) ? base_window[10] : base_window[11];
			const seed_t min_6 =
			    (base_window[12].hash <= base_window[13].hash) ? base_window[12] : base_window[13];
			const seed_t min_7 =
			    (base_window[14].hash <= base_window[15].hash) ? base_window[14] : base_window[15];
			const seed_t min_8 =
			    (base_window[16].hash <= base_window[17].hash) ? base_window[16] : base_window[17];
			const seed_t min_9  = (min_0.hash <= min_1.hash) ? min_0 : min_1;
			const seed_t min_10 = (min_2.hash <= min_3.hash) ? min_2 : min_3;
			const seed_t min_11 = (min_4.hash <= min_5.hash) ? min_4 : min_5;
			const seed_t min_12 = (min_6.hash <= min_7.hash) ? min_6 : min_7;
			const seed_t min_13 = (min_8.hash <= base_window[18].hash) ? min_8 : base_window[18];
			const seed_t min_14 = (min_9.hash <= min_10.hash) ? min_9 : min_10;
			const seed_t min_15 = (min_11.hash <= min_12.hash) ? min_11 : min_12;
			const seed_t min_16 = (min_13.hash <= min_14.hash) ? min_13 : min_14;
			const seed_t min_17 = (min_15.hash <= min_16.hash) ? min_15 : min_16;
			min                 = min_17;

			// std::cout << "min hash: " << min.hash << " loc: " << min.loc << std::endl;
			// std::cout << "prev hash: " << previous_minimizer.hash << " loc: " << previous_minimizer.loc
			//<< std::endl;

			EOR = 0;
		}

		// Write
		if (length > SE_K + SE_W - 2 || EOR) {
			if (EOR) {
				minimizers_o << previous_minimizer;
			} else if (min.loc != previous_minimizer.loc) {
				// std::cout << "hash: " << min.hash << std::endl;
				minimizers_o << min;
				previous_minimizer = min;
			}
			EOR = 0;
		}
	}
	minimizers_o << seed_t{.hash = MAX, .loc = 0, .str = 1, .EOR = 1};
}
