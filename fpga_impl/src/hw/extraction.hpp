#ifndef EXTRACTION_HPP
#define EXTRACTION_HPP
#include "kernel.hpp"
void extract_seeds(const base_t *read_i, seed_t *p0_o, seed_t *p1_o);
void push_seed(seed_t *p, seed_t *p0_o, seed_t *p1_o, ap_uint<SEED_BUF_LEN_LOG> &p_l, hash_t val, size_t &p_j);
#endif
