#include "extraction.hpp"
#include <stddef.h>
#define SHIFT1 (2 * (K - 1))

static inline ap_uint<2 * K> hash64(ap_uint<2 * K> key) {
	key = (~key + (key << 21)); // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)); // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)); // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31));
	return key;
}

void extract_minimizers(const base_t *read_i, min_stra_b_t *p0_o, min_stra_b_t *p1_o) {
	ap_uint<MIN_STRA_SIZE_LOG> p_l(0);
	min_stra_t buff[W];
	ap_uint<2 * K> kmer[2] = {0, 0};
	ap_uint<READ_LEN_LOG> l(0); // l counts the number of bases and is reset
	                            // to 0 each time there is an ambiguous base
	ap_uint<W_LOG> buff_pos(0);
	ap_uint<W_LOG> min_pos(0);
	min_stra_t min_reg = {MAX_KMER, 0};
	ap_uint<1> min_saved(0);
	ap_uint<1> same_min(0);
	min_stra_b_t p[MIN_STRA_SIZE];
LOOP_extract_minimizer:
	for (size_t i = 0; i < READ_LEN; ++i) {
		base_t c            = read_i[i];
		min_stra_t hash_reg = {MAX_KMER, 0};
		if (c < 4) {                                                 // not an ambiguous base
			kmer[0] = (kmer[0] << 2 | c);                        // forward k-mer
			kmer[1] = (kmer[1] >> 2) | (MAX_KMER ^ c) << SHIFT1; // reverse k-mer
			if (kmer[0] == kmer[1]) {
				continue; // skip "symmetric k-mers" as we
				          // don't know it strand
			}
			ap_uint<1> z = (kmer[0] > kmer[1]); // strand
			++l;
			if (l >= K) {
				hash_reg.minimizer = hash64(kmer[z]);
				hash_reg.strand    = z;
			}
		} else {
			if (l >= W + K - 1) {
				push_min_stra(p, p0_o, p1_o, p_l, min_reg);
				min_saved = 1;
			}
			l = 0;
		}
		buff[buff_pos] = hash_reg;
		if (l == W + K - 1) {
			if (same_min) {
				push_min_stra(p, p0_o, p1_o, p_l, (min_stra_t){min_reg.minimizer, !min_reg.strand});
				same_min = 0;
			}
		}
		if (hash_reg.minimizer <= min_reg.minimizer) {
			if (l >= W + K) {
				push_min_stra(p, p0_o, p1_o, p_l, min_reg);
			} else if (l < W + K - 1 && hash_reg.minimizer == min_reg.minimizer &&
			           hash_reg.strand != min_reg.strand) {
				same_min = 1;
			} else {
				same_min = 0;
			}
			min_reg   = hash_reg;
			min_pos   = buff_pos;
			min_saved = 0;
		} else if (buff_pos == min_pos) {
			if (l >= W + K - 1) {
				push_min_stra(p, p0_o, p1_o, p_l, min_reg);
			}
			min_reg.minimizer     = MAX_KMER;
			ap_uint<1> same_min_w = 0;
		LOOP_new_min:
			for (size_t j = 0; j < W; j++) {
#pragma HLS PIPELINE II = 1
				size_t buff_i = (j >= W - buff_pos.to_uint() - 1) ? j - W + buff_pos.to_uint() + 1
				                                                  : j + buff_pos.to_uint() + 1;
				if (min_reg.minimizer > buff[buff_i].minimizer) {
					min_reg    = buff[buff_i];
					min_pos    = buff_i;
					min_saved  = 0;
					same_min_w = 0;
				} else if (min_reg.minimizer == buff[buff_i].minimizer &&
				           min_reg.minimizer != MAX_KMER) {
					min_pos = buff_i;
					if (min_reg.strand != buff[buff_i].strand) {
						same_min_w = 1;
					}
				}
			}
			if (same_min_w && l >= W + K - 1) {
				push_min_stra(p, p0_o, p1_o, p_l, (min_stra_t){min_reg.minimizer, !min_reg.strand});
			}
		}
		buff_pos = (buff_pos == W - 1) ? 0 : buff_pos.to_uint() + 1;
	}
	if (min_reg.minimizer != MAX_KMER && !min_saved) {
		push_min_stra(p, p0_o, p1_o, p_l, min_reg);
	}
	p0_o[p_l] = END_MINIMIZER;
	p1_o[p_l] = END_MINIMIZER;
}

void push_min_stra(min_stra_b_t *p, min_stra_b_t *p0_o, min_stra_b_t *p1_o, ap_uint<MIN_STRA_SIZE_LOG> &p_l,
                   min_stra_t val) {
	min_stra_b_t min_stra = {val.minimizer, val.strand, 1};
	ap_uint<1> flag(1);
	ap_uint<MIN_STRA_SIZE_LOG> i(0);
	p[p_l] = min_stra;
LOOP_push_min_stra:
	for (size_t i = 0; i < MIN_STRA_SIZE; i++) {
#pragma HLS unroll
		if (p[i] == min_stra && i < p_l) {
			flag = 0;
		}
	}
	if (flag) {
		p0_o[p_l] = min_stra;
		p1_o[p_l] = min_stra;
		p_l++;
	}
}
