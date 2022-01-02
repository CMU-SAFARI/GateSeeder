#ifndef EXTRACTION_HPP
#define EXTRACTION_HPP
#include "kernel.hpp"
void extract_seeds(hls::stream<base_t> &seq_i, hls::stream<seed_t> &p0_o, hls::stream<seed_t> &p1_o);
void push_seed(seed_t *p, hls::stream<seed_t> &p0_o, hls::stream<seed_t> &p1_o, ap_uint<SEED_BUF_LEN_LOG> &p_l,
               hash_t val);
#endif
