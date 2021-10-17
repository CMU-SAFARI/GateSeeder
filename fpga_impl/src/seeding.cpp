#include "seeding.h"
#include "extraction.h"
#include <string.h>

void seeding(const uint32_t h[H_SIZE], const uint32_t location[LS_SIZE],
             const base_t *read, uint32_t *dst) {

#pragma HLS INTERFACE mode = m_axi port = h bundle = index_mem
#pragma HLS INTERFACE mode = m_axi port = location bundle = index_mem

#pragma HLS INTERFACE mode = m_axi port = read depth = 100 // LEN_READ
#pragma HLS INTERFACE mode = m_axi port = dst depth = 100  // LEN_READ

    base_t read_buff[READ_LEN];

    // memcpy creates a burst access to memory
    memcpy((void *)read_buff, (const void *)read, READ_LEN * sizeof(base_t));

    min_stra_v p; // Buffer which stores the minimizers and their strand
    p.n = 0;

    extract_minimizers(read, &p);
}
