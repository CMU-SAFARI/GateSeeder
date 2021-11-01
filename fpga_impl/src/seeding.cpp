#include "seeding.hpp"
#include "extraction.hpp"
#include <string.h>

void seeding(const ap_uint<32> h[H_SIZE], const ap_uint<32> location[LS_SIZE], const base_t *read_i,
             ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_len_o) {

#pragma HLS INTERFACE mode = m_axi port = h bundle = h
#pragma HLS INTERFACE mode = m_axi port = location bundle = loc // F
#pragma HLS INTERFACE mode = m_axi port = read_i depth = 100    // LEN_READ & LEN_READ
#pragma HLS INTERFACE mode = m_axi port = locs_o depth = 5000   // OUT_SIZE TODO
#pragma HLS dataflow
	base_t read_buff[READ_LEN];
	min_stra_v p; // Buffer which stores the minimizers and their strand
	ap_uint<32> location_buffer1[LOCATION_BUFFER_SIZE];
	ap_uint<32> location_buffer2[LOCATION_BUFFER_SIZE];
	ap_uint<LOCATION_BUFFER_SIZE_LOG> location_buffer1_len(0);
	ap_uint<LOCATION_BUFFER_SIZE_LOG> location_buffer2_len(0);
	ap_uint<1> buffer_sel;
	ap_uint<32> locs_buffer[LOCATION_BUFFER_SIZE];

	memcpy((void *)read_buff, (const void *)read_i, READ_LEN * sizeof(base_t));
	extract_minimizers(read_buff, &p);
	loop_query_locations(p, location_buffer1, location_buffer2, location_buffer1_len, location_buffer2_len,
	                     buffer_sel, h, location);

	if (buffer_sel) {
		adjacency_test(location_buffer1, location_buffer1_len, locs_buffer, locs_len_o);
	} else {
		adjacency_test(location_buffer2, location_buffer2_len, locs_buffer, locs_len_o);
	}

	if (locs_len_o) {
		memcpy((void *)locs_o, (const void *)locs_buffer, locs_len_o * sizeof(ap_uint<32>));
	}
}

void get_locations(const min_stra_b_t min_stra, ap_uint<32> *mem_buffer, ap_uint<F_LOG> &len, const ap_uint<32> *h,
                   const ap_uint<32> *location) {
	ap_uint<32> minimizer = min_stra.minimizer;
	ap_uint<32> min       = minimizer ? h[minimizer - 1].to_uint() : 0;
	ap_uint<32> max       = h[minimizer];
	len                   = max - min;
LOOP_read_locations:
	for (size_t j = 0; j < len; j++) {
#pragma HLS loop_tripcount min = 0 max = 500 avg = 5 // F & AVG_LOC
#pragma HLS PIPELINE II                          = 1

		mem_buffer[j] = location[min + j] ^ min_stra.strand;
	}
}

void merge_locations(const ap_uint<32> *buffer_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer_i_len,
                     ap_uint<32> *buffer_o, ap_uint<LOCATION_BUFFER_SIZE_LOG> &buffer_o_len,
                     const ap_uint<32> *mem_buffer, const ap_uint<F_LOG> mem_buffer_len) {
	size_t loc_i = 0;
	size_t mem_i = 0;
	buffer_o_len = 0;
LOOP_merge_fisrt_part:
	while (loc_i < buffer_i_len && mem_i < mem_buffer_len) {
#pragma HLS loop_tripcount min = 0 max = 500 // F
#pragma HLS PIPELINE II                = 1

		if (buffer_i[loc_i] <= mem_buffer[mem_i]) {
			buffer_o[buffer_o_len] = buffer_i[loc_i];
			loc_i++;
		} else {
			buffer_o[buffer_o_len] = mem_buffer[mem_i];
			mem_i++;
		}
		buffer_o_len++;
	}
	if (loc_i == buffer_i_len) {
	LOOP_merge_second_part:
		for (; mem_i < mem_buffer_len; mem_i++) {
#pragma HLS loop_tripcount min = 0 max = 500 // F
#pragma HLS PIPELINE II                = 1

			buffer_o[buffer_o_len] = mem_buffer[mem_i];
			buffer_o_len++;
		}
	} else {
	LOOP_merge_third_part:
		for (; loc_i < buffer_i_len; loc_i++) {
#pragma HLS loop_tripcount min = 0 max = 70000 // LOCATION_BUFFER_SIZE
#pragma HLS PIPELINE II                = 1

			buffer_o[buffer_o_len] = buffer_i[loc_i];
			buffer_o_len++;
		}
	}
}

void inline query_locations(const min_stra_b_t min_stra, ap_uint<32> *mem_buffer_w, ap_uint<F_LOG> &mem_buffer_len_w,
                            const ap_uint<32> *h, const ap_uint<32> *location, const ap_uint<32> *buffer_i,
                            const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer_i_len, ap_uint<32> *buffer_o,
                            ap_uint<LOCATION_BUFFER_SIZE_LOG> &buffer_o_len, const ap_uint<32> *mem_buffer_r,
                            const ap_uint<F_LOG> mem_buffer_len_r) {
#pragma HLS dataflow
	get_locations(min_stra, mem_buffer_w, mem_buffer_len_w, h, location);
	merge_locations(buffer_i, buffer_i_len, buffer_o, buffer_o_len, mem_buffer_r, mem_buffer_len_r);
}

void inline loop_query_locations(const min_stra_v p, ap_uint<32> *location_buffer1, ap_uint<32> *location_buffer2,
                                 ap_uint<LOCATION_BUFFER_SIZE_LOG> &location_buffer1_len,
                                 ap_uint<LOCATION_BUFFER_SIZE_LOG> &location_buffer2_len, ap_uint<1> &buffer_sel,
                                 const ap_uint<32> *h, const ap_uint<32> *location) {
	ap_uint<32> mem_buffer1[F];
	ap_uint<32> mem_buffer2[F];
	ap_uint<F_LOG> mem_buffer1_len;
	ap_uint<F_LOG> mem_buffer2_len;
	get_locations(p.a[0], mem_buffer1, mem_buffer1_len, h, location);
LOOP_query_locations:
	for (size_t i = 1; i < p.n; i++) {
#pragma HLS loop_tripcount min = 0 max = 100 // READ_LEN
		if (i % 2) {
			query_locations(p.a[i], mem_buffer2, mem_buffer2_len, h, location, location_buffer1,
			                location_buffer1_len, location_buffer2, location_buffer2_len, mem_buffer1,
			                mem_buffer1_len);
		} else {
			query_locations(p.a[i], mem_buffer1, mem_buffer1_len, h, location, location_buffer2,
			                location_buffer2_len, location_buffer1, location_buffer1_len, mem_buffer2,
			                mem_buffer2_len);
		}
	}
	if (p.n % 2) {
		merge_locations(location_buffer1, location_buffer1_len, location_buffer2, location_buffer2_len,
		                mem_buffer1, mem_buffer1_len);
		buffer_sel = 0;
	} else {
		merge_locations(location_buffer2, location_buffer2_len, location_buffer1, location_buffer1_len,
		                mem_buffer2, mem_buffer2_len);
		buffer_sel = 1;
	}
}

void adjacency_test(const ap_uint<32> *buffer_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer_i_len,
                    ap_uint<32> *buffer_o, ap_uint<OUT_SIZE_LOG> &buffer_o_len) {
	ap_uint<32> buffer1[MIN_T_1];
	ap_uint<32> buffer2[MIN_T_1];
	ap_uint<MIN_T_LOG> ready1(0);
	ap_uint<MIN_T_LOG> ready2(0);
	buffer_o_len = 0;
	for (size_t i = 0; i < buffer_i_len; i++) {
#pragma HLS loop_tripcount min = 0 max = 70000 // LOCATION_BUFFER_SIZE
#pragma HLS PIPELINE II                = 1

		ap_uint<32> loc = buffer_i[i];
		if (loc[0]) {
			if (ready1 == MIN_T_1) {
				if (loc - buffer1[0] < LOC_R) {
					buffer_o[buffer_o_len] = (buffer1[0].range(31, 1), ap_uint<1>(0));
					buffer_o_len++;
				}
			} else {
				ready1++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buffer1[j] = buffer1[j + 1];
			}
			buffer1[MIN_T_1 - 1] = loc;
		} else {
			if (ready2 == MIN_T_1) {
				if (loc - buffer2[0] < LOC_R) {
					buffer_o[buffer_o_len] = buffer2[0];
					buffer_o_len++;
				}
			} else {
				ready2++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buffer2[j] = buffer2[j + 1];
			}
			buffer2[MIN_T_1 - 1] = loc;
		}
	}
}
