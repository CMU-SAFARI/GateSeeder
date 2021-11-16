#ifndef EXTRACTION_HPP
#define EXTRACTION_HPP

#include "seeding.hpp"

void extract_minimizers(const base_t *read_i, min_stra_b_t *p_o);
void push_min_stra(min_stra_b_t *p, min_stra_b_t *p_o, ap_uint<MIN_STRA_SIZE_LOG> &p_l, min_stra_t val);
#endif
