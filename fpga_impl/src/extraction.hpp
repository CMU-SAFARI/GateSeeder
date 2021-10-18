#ifndef EXTRACTION_HPP
#define EXTRACTION_HPP

#include "ap_int.h"
#include "seeding.hpp"

#define MIN_STRA_SIZE 5000   // TODO
#define MIN_STRA_SIZE_LOG 13 // TODO

struct min_stra_b_t {
    ap_uint<B> minimizer;
    ap_uint<1> strand;
    int operator==(min_stra_b_t x) {
        return (this->minimizer == x.minimizer && this->strand == x.strand);
    }
};

struct min_stra_t {
    ap_uint<2 * K> minimizer;
    ap_uint<1> strand;
};

struct min_stra_v {
    ap_uint<MIN_STRA_SIZE_LOG> n;
    min_stra_b_t a[MIN_STRA_SIZE];
    ap_uint<READ_LEN_LOG> repetition[MIN_STRA_SIZE];
};

void extract_minimizers(const base_t *read, min_stra_v *p);
void push_min_stra(min_stra_v *p, min_stra_t val);

#endif
