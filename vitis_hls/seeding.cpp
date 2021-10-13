#include "seeding.h"
#define W 12
#define K 18
#define MIN_T 3
#define LOC_R 200
#define B 26
#define LEN_READ 100

void seeding(const uint32_t h[H_SIZE], const uint32_t location[LS_SIZE],
             const ap_uint<1> strand[LS_SIZE], const base_t *read,
             uint32_t *dst) {
#pragma HLS INTERFACE mode = ap_memory port = h
#pragma HLS INTERFACE mode = ap_memory port = location
#pragma HLS INTERFACE mode = ap_memory port = strand

#pragma HLS INTERFACE m_axi port = read depth = 100 // LEN_READ
#pragma HLS INTERFACE m_axi port = dst depth = 100  // LEN_READ
}
