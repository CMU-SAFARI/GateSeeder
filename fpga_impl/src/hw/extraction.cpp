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

void extract_seeds(hls::stream<base_t> &seq_i, hls::stream<seed_t> &p0_o, hls::stream<seed_t> &p1_o) {
	hash_t buf[W];
	ap_uint<2 * K> kmer[2] = {0, 0};
	ap_uint<READ_LEN_LOG> l(0); // l counts the number of bases and is reset
	                            // to 0 each time there is an ambiguous base
	ap_uint<W_LOG> buf_pos(0);
	ap_uint<W_LOG> seed_pos(0);

	hash_t min_hash_reg = {MAX_HASH, 0};

	ap_uint<1> seed_saved(0);
	ap_uint<1> same_hash(0);

	seed_t p[SEED_BUF_LEN];
	ap_uint<SEED_BUF_LEN_LOG> p_l(0);
#ifdef VARIABLE_LEN
	for (size_t i = 0; i < MAX_IN_LEN; ++i) {
		base_t c = read_i[i];
		if (c == END_OF_READ) break;
#else
	ap_uint<READ_LEN_LOG> base_j = 0;
	base_t c;
LOOP_extract_seeds:
	//TODO: set the number of bases
	for (size_t i = 0; i < MAX_IN_LEN; ++i) {
		if (base_j == READ_LEN) {
			l = 0;
			buf_pos = 0;
			seed_pos = 0;
			min_hash_reg = {MAX_HASH, 0};
			seed_saved = 0;
			same_hash = 0;
			p_l = 0;
			base_j = 0;
		} else {
			base_j++;
		}
		seq_i >> c;
		if (c == END_OF_SEQ_BASE) {
			p0_o << END_OF_SEQ_SEED;
			p1_o << END_OF_SEQ_SEED;
			break;
		}
#endif
		hash_t hash_reg = {MAX_HASH, 0};
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
				hash_reg.hash = hash64(kmer[z]);
				hash_reg.str  = z;
			}
		} else {
			if (l >= W + K - 1) {
				push_seed(p, p0_o, p1_o, p_l, min_hash_reg);
				seed_saved = 1;
			}
			l = 0;
		}
		buf[buf_pos] = hash_reg;
		if (l == W + K - 1) {
			if (same_hash) {
				push_seed(p, p0_o, p1_o, p_l, hash_t{min_hash_reg.hash, !min_hash_reg.str});
				same_hash = 0;
			}
		}
		if (hash_reg.hash <= min_hash_reg.hash) {
			if (l >= W + K) {
				push_seed(p, p0_o, p1_o, p_l, min_hash_reg);
			} else if (l < W + K - 1 && hash_reg.hash == min_hash_reg.hash &&
			           hash_reg.str != min_hash_reg.str) {
				same_hash = 1;
			} else {
				same_hash = 0;
			}
			min_hash_reg = hash_reg;
			seed_pos     = buf_pos;
			seed_saved   = 0;
		} else if (buf_pos == seed_pos) {
			if (l >= W + K - 1) {
				push_seed(p, p0_o, p1_o, p_l, min_hash_reg);
			}
			min_hash_reg.hash      = MAX_HASH;
			ap_uint<1> same_hash_w = 0;
		LOOP_new_min_hash:
			for (size_t j = 0; j < W; j++) {
#pragma HLS PIPELINE II = 1
				size_t buf_j = (j >= W - buf_pos.to_uint() - 1) ? j - W + buf_pos.to_uint() + 1
				                                                : j + buf_pos.to_uint() + 1;
				if (min_hash_reg.hash > buf[buf_j].hash) {
					min_hash_reg = buf[buf_j];
					seed_pos     = buf_j;
					seed_saved   = 0;
					same_hash_w  = 0;
				} else if (min_hash_reg.hash == buf[buf_j].hash && min_hash_reg.hash != MAX_HASH) {
					seed_pos = buf_j;
					if (min_hash_reg.str != buf[buf_j].str) {
						same_hash_w = 1;
					}
				}
			}
			if (same_hash_w && l >= W + K - 1) {
				push_seed(p, p0_o, p1_o, p_l, hash_t{min_hash_reg.hash, !min_hash_reg.str});
			}
		}
		buf_pos = (buf_pos == W - 1) ? 0 : buf_pos.to_uint() + 1;
	}
	if (min_hash_reg.hash != MAX_HASH && !seed_saved) {
		push_seed(p, p0_o, p1_o, p_l, min_hash_reg);
	}
	p0_o << END_OF_READ_SEED;
	p1_o << END_OF_READ_SEED;
}

void push_seed(seed_t *p, hls::stream<seed_t> &p0_o, hls::stream<seed_t> &p1_o, ap_uint<SEED_BUF_LEN_LOG> &p_l,
               hash_t val) {
	seed_t seed = {val.hash, val.str, 1};
	ap_uint<1> flag(1);
	p[p_l] = seed;
LOOP_push_seed:
	for (size_t i = 0; i < SEED_BUF_LEN; i++) {
#pragma HLS unroll
		if (p[i] == seed && i < p_l) {
			flag = 0;
		}
	}
	if (flag) {
		p0_o << seed;
		p1_o << seed;
		p_l++;
	}
}
