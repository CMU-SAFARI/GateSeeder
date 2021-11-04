#include "seeding.hpp"
#include "extraction.hpp"
#include <stddef.h>

void seeding(const ap_uint<32> h[H_SIZE], const ap_uint<32> location[LS_SIZE], const base_t *read_i,
             ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_len_o) {

#pragma HLS INTERFACE mode = m_axi port = h bundle = h
#pragma HLS INTERFACE mode = m_axi port = location bundle = loc // F
#pragma HLS INTERFACE mode = m_axi port = read_i depth = 100    // LEN_READ & LEN_READ
#pragma HLS INTERFACE mode = m_axi port = locs_o depth = 5000   // OUT_SIZE TODO
#pragma HLS INTERFACE mode = s_axilite port = locs_len_o
#pragma HLS dataflow
	base_t read[READ_LEN];
	min_stra_b_t p[MIN_STRA_SIZE]; // Buffer which stores the minimizers
	                               // and their strand
	ap_uint<MIN_STRA_SIZE_LOG> p_l;
	ap_uint<32> locs[OUT_SIZE];
	ap_uint<OUT_SIZE_LOG> locs_len;

	read_read(read, read_i);
	extract_minimizers(read, p, p_l);
	get_locations(p, p_l, h, location, locs, locs_len);
	write_locations(locs_o, locs, locs_len, locs_len_o);
}

void read_read(base_t *read_buff, const base_t *read_i) {
LOOP_read_read:
	for (size_t i = 0; i < READ_LEN; i++) {
#pragma HLS loop_tripcount min = 100 max = 100 // READ_LEN
#pragma HLS PIPELINE II                  = 1

		read_buff[i] = read_i[i];
	}
}

void write_locations(ap_uint<32> *locs_o, const ap_uint<32> *locs_buffer, const ap_uint<OUT_SIZE_LOG> locs_len,
                     ap_uint<OUT_SIZE_LOG> &locs_len_o) {
LOOP_write_locs:
	for (size_t i = 0; i < locs_len; i++) {
#pragma HLS loop_tripcount min = 0 max = 5000 // OUT_SIZE
#pragma HLS PIPELINE II                = 1

		locs_o[i] = locs_buffer[i];
	}
	locs_len_o = locs_len;
}

void get_locations(const min_stra_b_t *p_i, const ap_uint<MIN_STRA_SIZE_LOG> p_li, const ap_uint<32> *h,
                   const ap_uint<32> *location, ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_len_o) {
	ap_uint<32> location_buffer1[LOCATION_BUFFER_SIZE];
	ap_uint<32> location_buffer2[LOCATION_BUFFER_SIZE];
	ap_uint<LOCATION_BUFFER_SIZE_LOG> location_buffer1_len(0);
	ap_uint<LOCATION_BUFFER_SIZE_LOG> location_buffer2_len(0);
	ap_uint<32> mem_buffer1[F];
	ap_uint<32> mem_buffer2[F];
	ap_uint<F_LOG> mem_buffer1_len;
	ap_uint<F_LOG> mem_buffer2_len;
	for (size_t i = 0; i < p_li / 2; i++) {
#pragma HLS loop_tripcount min = 0 max = 50 // READ_LEN / 2
		read_locations(p_i[2 * i], mem_buffer1, mem_buffer1_len, h, location);
		merge_locations(location_buffer1, location_buffer1_len, location_buffer2, location_buffer2_len,
		                mem_buffer1, mem_buffer1_len);
		read_locations(p_i[2 * i + 1], mem_buffer2, mem_buffer2_len, h, location);
		merge_locations(location_buffer2, location_buffer2_len, location_buffer1, location_buffer1_len,
		                mem_buffer2, mem_buffer2_len);
	}
	if (p_li % 2) {
		read_locations(p_i[p_li - 1], mem_buffer1, mem_buffer1_len, h, location);
		merge_locations(location_buffer1, location_buffer1_len, location_buffer2, location_buffer2_len,
		                mem_buffer1, mem_buffer1_len);
		adjacency_test(location_buffer2, location_buffer2_len, locs_o, locs_len_o);
	} else {
		adjacency_test(location_buffer1, location_buffer1_len, locs_o, locs_len_o);
	}
}

void read_locations(const min_stra_b_t min_stra, ap_uint<32> *mem_buffer, ap_uint<F_LOG> &len, const ap_uint<32> *h,
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

void adjacency_test(const ap_uint<32> *buffer_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer_len_i,
                    ap_uint<32> *buffer_o, ap_uint<OUT_SIZE_LOG> &buffer_len_o) {
	ap_uint<32> buffer1[MIN_T_1];
	ap_uint<32> buffer2[MIN_T_1];
	ap_uint<MIN_T_LOG> ready1(0);
	ap_uint<MIN_T_LOG> ready2(0);
	buffer_len_o = 0;
	for (size_t i = 0; i < buffer_len_i; i++) {
#pragma HLS loop_tripcount min = 0 max = 70000 // LOCATION_BUFFER_SIZE
#pragma HLS PIPELINE II                = 1

		ap_uint<32> loc = buffer_i[i];
		if (loc[0]) {
			if (ready1 == MIN_T_1) {
				if (loc - buffer1[0] < LOC_R) {
					buffer_o[buffer_len_o] = (buffer1[0].range(31, 1), ap_uint<1>(0));
					buffer_len_o++;
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
					buffer_o[buffer_len_o] = buffer2[0];
					buffer_len_o++;
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
