#ifndef SEEDING_HPP
#define SEEDING_HPP

#include "ap_int.h"
#include <stdint.h>

#define H_SIZE 67108864  // chrom1_w12_k18_f500_b26
#define LS_SIZE 33594136 // chrom1_w12_k18_f500_b26
#define W 12
#define W_LOG 4
#define K 18
#define B 26
#define MIN_T 3
#define LOC_R 200
#define READ_LEN 100
#define READ_LEN_LOG 7
#define LOCATION_BUFFER_SIZE 200000 // TODO
#define OUT 5000
#define OUT_LOG 13

typedef ap_uint<3> base_t;

void seeding(const ap_uint<32> h[H_SIZE], const ap_uint<32> location[LS_SIZE],
             const base_t *read_i, ap_uint<32> *locs_o,
             ap_uint<OUT_LOG> &locs_len_o);
#endif
