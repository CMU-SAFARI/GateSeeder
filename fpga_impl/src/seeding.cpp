#include "seeding.hpp"
#include "extraction.hpp"
#include <string.h>

void seeding(const ap_uint<32> h[H_SIZE], const ap_uint<32> location[LS_SIZE],
             const base_t *read_i, ap_uint<32> *locs_o,
             ap_uint<OUT_LOG> &locs_len_o) {

#pragma HLS INTERFACE mode = m_axi port = h bundle = index_mem
#pragma HLS INTERFACE mode = m_axi port = location bundle = index_mem

#pragma HLS INTERFACE mode = m_axi port = read_i depth = 100  // LEN_READ
#pragma HLS INTERFACE mode = m_axi port = locs_o depth = 5000 // OUT TODO

    base_t read_buff[READ_LEN];

    // memcpy creates a burst access to memory
    memcpy((void *)read_buff, (const void *)read_i, READ_LEN * sizeof(base_t));

    min_stra_v p; // Buffer which stores the minimizers and their strand
    p.n = 0;

    extract_minimizers(read_buff, &p);
    ap_uint<32> location_buffer[2]
                               [LOCATION_BUFFER_SIZE]; // Buffers which stores
    // the locations
    size_t location_buffer_len[2] = {0}; // TODO
    ap_uint<32> mem_buffer[2][2000];     // Buffers used to store the locations
                                         // returned by the index
    uint16_t mem_buffer_len[2];          // TODO
    uint8_t repetition[2];               // TODO

    for (size_t i = 0; i <= p.n; i++) {
        unsigned char sel = i % 2;
        // Query the locations and the strands from the index and store them
        // into one of mem_buffer
        if (i != p.n) {
            size_t mem_buffer_i = 0;
            uint32_t minimizer = p.a[i].minimizer;
            uint32_t min = minimizer ? h[minimizer - 1].to_uint() : 0;
            uint32_t max = h[minimizer];
            repetition[sel] = p.repetition[sel];
            mem_buffer_len[sel] = max - min;
            for (uint32_t j = min; j < max; j++) {
                mem_buffer[sel][mem_buffer_i] = location[j] ^ p.a[i].strand;

                mem_buffer_i++;
            }
        }
        // Merge the previously loaded mem_buffer with one of the
        // location_buffer in the other location_buffer
        if (i != 0) {
            size_t loc_i = 0;
            size_t mem_i = 0;
            size_t len = 0;
            while (loc_i < location_buffer_len[sel] &&
                   mem_i < mem_buffer_len[1 - sel]) {
                if (location_buffer[sel][loc_i] <= mem_buffer[1 - sel][mem_i]) {
                    location_buffer[1 - sel][len] = location_buffer[sel][loc_i];
                    len++;
                    loc_i++;
                } else {
                    for (uint8_t rep = 0; rep < repetition[1 - sel]; rep++) {
                        location_buffer[1 - sel][len] =
                            mem_buffer[1 - sel][mem_i];
                        len++;
                    }
                    mem_i++;
                }
            }
            if (loc_i == location_buffer_len[sel]) {
                while (mem_i != mem_buffer_len[1 - sel]) {
                    for (uint8_t rep = 0; rep < repetition[1 - sel]; rep++) {
                        location_buffer[1 - sel][len] =
                            mem_buffer[1 - sel][mem_i];
                        len++;
                    }
                    mem_i++;
                }
            } else {
                while (loc_i != location_buffer_len[sel]) {
                    location_buffer[1 - sel][len] = location_buffer[sel][loc_i];
                    len++;
                    loc_i++;
                }
            }
            location_buffer_len[1 - sel] = len;
        }
    }

    ap_uint<32> *buffer = location_buffer[1 - p.n % 2];
    size_t n = location_buffer_len[1 - p.n % 2];
    ap_uint<OUT_LOG> locs_len;
    // Adjacency test
    if (n >= MIN_T) {
        ap_uint<32> loc_buffer[LOCATION_BUFFER_SIZE];
        locs_len = 0;
        unsigned char loc_counter = 1;
        size_t init_loc_idx = 0;
        while (init_loc_idx < n - MIN_T + 1) {
            if ((buffer[init_loc_idx + loc_counter] - buffer[init_loc_idx] <
                 LOC_R) &&
                buffer[init_loc_idx + loc_counter][0] ==
                    buffer[init_loc_idx][0]) {
                loc_counter++;
                if (loc_counter == MIN_T) {
                    loc_buffer[locs_len] = buffer[init_loc_idx];
                    locs_len++;
                    init_loc_idx++;
                    loc_counter = 1;
                }
            } else {
                init_loc_idx++;
                loc_counter = 1;
            }
        }

        // Store the positons
        memcpy((void *)locs_o, (const void *)loc_buffer,
               locs_len * sizeof(ap_uint<32>));
        locs_len_o = locs_len;
    } else {
        locs_len_o = 0;
    }
}
