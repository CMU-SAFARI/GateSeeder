#include "seeding.h"
#include <string.h>
#define W 12
#define K 18
#define MIN_T 3
#define LOC_R 200
#define B 26
#define LEN_READ 100

void seeding(const uint32_t h[H_SIZE], const uint32_t location[LS_SIZE],
             const ap_uint<1> strand[LS_SIZE], const base_t *read,
             uint32_t *dst) {
#pragma HLS INTERFACE mode = m_axi port = h
#pragma HLS INTERFACE mode = m_axi port = location
#pragma HLS INTERFACE mode = m_axi port = strand

#pragma HLS INTERFACE mode = m_axi port = read depth = 100 // LEN_READ
#pragma HLS INTERFACE mode = m_axi port = dst depth = 100  // LEN_READ

    base_t read_buff[LEN_READ];

    // memcpy creates a burst access to memory
    memcpy((void *)read_buff, (const void *)read, LEN_READ * sizeof(base_t));
}
