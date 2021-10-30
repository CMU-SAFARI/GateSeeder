#include "seeding.hpp"
#include "extraction.hpp"
#include <string.h>

void seeding(const ap_uint<32> h[H_SIZE], const ap_uint<32> location[LS_SIZE], const base_t *read_i,
             ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_len_o) {

#pragma HLS INTERFACE mode = m_axi port = h bundle = index_mem
#pragma HLS INTERFACE mode = m_axi port = location bundle = index_mem // F
#pragma HLS INTERFACE mode = m_axi port = read_i depth = 100          // LEN_READ & LEN_READ
#pragma HLS INTERFACE mode = m_axi port = locs_o depth = 5000         // OUT_SIZE TODO

	base_t read_buff[READ_LEN];

	// memcpy creates a burst access to memory
	memcpy((void *)read_buff, (const void *)read_i, READ_LEN * sizeof(base_t));

	min_stra_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;

	extract_minimizers(read_buff, &p);
	ap_uint<32> location_buffer[2][LOCATION_BUFFER_SIZE]; // Buffers which stores
	// the locations
	ap_uint<LOCATION_BUFFER_SIZE_LOG> location_buffer_len[2];
	location_buffer_len[0] = 0;
	location_buffer_len[1] = 0;

	ap_uint<32> mem_buffer[2][F];     // Buffers used to store the locations
	                                  // returned by the index
	ap_uint<F_LOG> mem_buffer_len[2]; // TODO

	ap_uint<1> sel(0);
LOOP_query_locations:
	for (size_t i = 0; i <= p.n; i++) {
		// Query the locations and the strands from the index and store them
		// into one of mem_buffer
		if (i != p.n) {
			ap_uint<32> minimizer = p.a[i].minimizer;
			ap_uint<32> min       = minimizer ? h[minimizer - 1].to_uint() : 0;
			ap_uint<32> max       = h[minimizer];
			mem_buffer_len[sel]   = max - min;
		LOOP_read_locations:
			for (size_t j = 0; j < mem_buffer_len[sel]; j++) {
#pragma HLS loop_tripcount min = 0 max = 500 avg = 5 // F & AVG_LOC
#pragma HLS PIPELINE II                          = 1

				mem_buffer[sel][j] = location[j + min] ^ p.a[i].strand;
			}
		}
		// Merge the previously loaded mem_buffer with one of the
		// location_buffer in the other location_buffer
		if (i != 0) {
			size_t loc_i = 0;
			size_t mem_i = 0;
			size_t len   = 0;
		LOOP_merge_fisrt_part:
			while (loc_i < location_buffer_len[sel] && mem_i < mem_buffer_len[!sel]) {
				if (location_buffer[sel][loc_i] <= mem_buffer[!sel][mem_i]) {
					location_buffer[!sel][len] = location_buffer[sel][loc_i];
					len++;
					loc_i++;
				} else {
					location_buffer[!sel][len] = mem_buffer[!sel][mem_i];
					len++;
					mem_i++;
				}
			}
			if (loc_i == location_buffer_len[sel]) {
			LOOP_merge_second_part:
				while (mem_i != mem_buffer_len[!sel]) {
					location_buffer[!sel][len] = mem_buffer[!sel][mem_i];
					len++;
					mem_i++;
				}
			} else {
			LOOP_merge_third_part:
				while (loc_i != location_buffer_len[sel]) {
					location_buffer[!sel][len] = location_buffer[sel][loc_i];
					len++;
					loc_i++;
				}
			}
			location_buffer_len[!sel] = len;
		}
		sel = !sel;
	}

	ap_uint<32> *buffer                 = location_buffer[sel];
	ap_uint<LOCATION_BUFFER_SIZE_LOG> n = location_buffer_len[sel];
	ap_uint<OUT_SIZE_LOG> locs_len;

	// Adjacency test
	if (n >= MIN_T) {
		ap_uint<32> loc_buffer[LOCATION_BUFFER_SIZE];
		locs_len                       = 0;
		ap_uint<MIN_T_LOG> loc_counter = 1;
		size_t loc_offset              = 1;
		size_t init_loc_idx            = 0;
	LOOP_adjacency_test:
		while (init_loc_idx < n - MIN_T + 1) {
			if ((buffer[init_loc_idx + loc_offset] - buffer[init_loc_idx] < LOC_R)) {
				if (buffer[init_loc_idx + loc_offset][0] == buffer[init_loc_idx][0]) {
					loc_counter++;
					if (loc_counter == MIN_T) {
						loc_buffer[locs_len] =
						    (buffer[init_loc_idx].range(31, 1), ap_uint<1>(0));
						locs_len++;
						init_loc_idx++;
						loc_counter = 1;
						loc_offset  = 0;
					}
				}
			} else {
				init_loc_idx++;
				loc_counter = 1;
				loc_offset  = 0;
			}
			loc_offset++;
		}

		// Store the positons
		memcpy((void *)locs_o, (const void *)loc_buffer, locs_len * sizeof(ap_uint<32>));
		locs_len_o = locs_len;
	} else {
		locs_len_o = 0;
	}
}
