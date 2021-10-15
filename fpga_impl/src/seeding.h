#ifndef SEEDING_H
#define SEEDING_H

#include "ap_int.h"
#include <stdint.h>

#define H_SIZE 67108864  // chrom1_w12_k18_f500_b26
#define LS_SIZE 33594136 // chrom1_w12_k18_f500_b26
typedef ap_uint<3> base_t;

void seeding(const uint32_t h[H_SIZE], const uint32_t location[LS_SIZE],
             const ap_uint<1> strand[LS_SIZE], const base_t *read,
             uint32_t *dst);
#endif
