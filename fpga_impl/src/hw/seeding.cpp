#include "seeding.hpp"
#include "extraction.hpp"
#include <stddef.h>

void seeding(const ap_uint<32> h_m[H_SIZE], const ap_uint<32> loc_stra_m[LS_SIZE], const base_t read_i[READ_LEN],
             ap_uint<32> locs_o[OUT_SIZE]) {
	//#pragma HLS INTERFACE m_axi port = h_m bundle = h_m
	//#pragma HLS INTERFACE m_axi port = loc_stra_m bundle = loc_stra_m
	//#pragma HLS INTERFACE m_axi port = read_i bundle = read_i
	//#pragma HLS INTERFACE m_axi port = locs_o depth = 5000 bundle = locs_o // OUT_SIZE TODO

#pragma HLS dataflow
	base_t read[READ_LEN];
	min_stra_b_t p[MIN_STRA_SIZE];
	ap_uint<32> locs[OUT_SIZE];
#pragma HLS STREAM variable = read
#pragma HLS STREAM variable = p depth = 2
#pragma HLS STREAM variable = locs depth = 4
	read_read(read_i, read);
	extract_minimizers(read, p);
	get_locations(p, h_m, loc_stra_m, locs);
	write_locations(locs, locs_o);
}

void read_read(const base_t *read_i, base_t *read_o) {
LOOP_read_read:
	for (size_t i = 0; i < READ_LEN; i++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 100 max = 100 // READ_LEN
		read_o[i] = read_i[i];
	}
}

void get_locations(const min_stra_b_t *p_i, const ap_uint<32> *h_m, const ap_uint<32> *loc_stra_m,
                   ap_uint<32> *locs_o) {
	ap_uint<32> loc_stra1[LOCATION_BUFFER_SIZE];
	ap_uint<32> loc_stra2[LOCATION_BUFFER_SIZE];
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra1_l(0);
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra2_l(0);
	ap_uint<32> buf1[F];
	ap_uint<32> buf2[F];
	ap_uint<F_LOG> buf1_l;
	ap_uint<F_LOG> buf2_l;
	ap_uint<1> flag;
	min_stra_b_t min_stra_buf(p_i[0]);
	if (min_stra_buf.valid == 1) {
		read_locations(min_stra_buf, buf1, buf1_l, h_m, loc_stra_m);
	LOOP_get_locations:
		for (size_t i = 0; i < READ_LEN; i += 2) {
			min_stra_buf = p_i[i + 1];
			if (min_stra_buf.valid == 0) {
				flag = 1;
				break;
			}
			merge_locations(loc_stra1, loc_stra1_l, buf1, buf1_l, loc_stra2, loc_stra2_l);
			read_locations(min_stra_buf, buf2, buf2_l, h_m, loc_stra_m);
			min_stra_buf = p_i[i + 2];
			if (min_stra_buf.valid == 0) {
				flag = 0;
				break;
			}
			merge_locations(loc_stra2, loc_stra2_l, buf2, buf2_l, loc_stra1, loc_stra1_l);
			read_locations(min_stra_buf, buf1, buf1_l, h_m, loc_stra_m);
		}
		if (flag) {
			merge_locations(loc_stra1, loc_stra1_l, buf1, buf1_l, loc_stra2, loc_stra2_l);
			adjacency_test(loc_stra2, loc_stra2_l, locs_o);
		} else {
			merge_locations(loc_stra2, loc_stra2_l, buf2, buf2_l, loc_stra1, loc_stra1_l);
			adjacency_test(loc_stra1, loc_stra1_l, locs_o);
		}
	}
}

void read_locations(const min_stra_b_t min_stra_i, ap_uint<32> *buf_o, ap_uint<F_LOG> &buf_lo, const ap_uint<32> *h_m,
                    const ap_uint<32> *loc_stra_m) {
#pragma HLS INLINE OFF
	ap_uint<32> minimizer = min_stra_i.minimizer;
	ap_uint<32> min       = minimizer ? h_m[minimizer - 1].to_uint() : 0;
	ap_uint<32> max       = h_m[minimizer];
	buf_lo                = max - min;
LOOP_read_locations:
	for (size_t j = 0; j < buf_lo; j++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = 500 avg = 5 // F & AVG_LOC
		buf_o[j] = loc_stra_m[min + j] ^ min_stra_i.strand;
	}
}

void merge_locations(const ap_uint<32> *loc_stra_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_li,
                     const ap_uint<32> *buf_i, const ap_uint<F_LOG> buf_li, ap_uint<32> *loc_stra_o,
                     ap_uint<LOCATION_BUFFER_SIZE_LOG> &loc_stra_lo) {
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_j = 0;
	ap_uint<F_LOG> buf_j                         = 0;
	loc_stra_lo                                  = 0;
LOOP_merge_fisrt_part:
	while (loc_stra_j < loc_stra_li && buf_j < buf_li) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = 500 // F
		if (loc_stra_i[loc_stra_j] <= buf_i[buf_j]) {
			loc_stra_o[loc_stra_lo] = loc_stra_i[loc_stra_j];
			loc_stra_j++;
		} else {
			loc_stra_o[loc_stra_lo] = buf_i[buf_j];
			buf_j++;
		}
		loc_stra_lo++;
	}
	if (loc_stra_j == loc_stra_li) {
	LOOP_merge_second_part:
		for (; buf_j < buf_li; buf_j++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = 500 // F
			loc_stra_o[loc_stra_lo] = buf_i[buf_j];
			loc_stra_lo++;
		}
	} else {
	LOOP_merge_third_part:
		for (; loc_stra_j < loc_stra_li; loc_stra_j++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = 70000 // LOCATION_BUFFER_SIZE
			loc_stra_o[loc_stra_lo] = loc_stra_i[loc_stra_j];
			loc_stra_lo++;
		}
	}
}

void adjacency_test(const ap_uint<32> *loc_stra_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_li,
                    ap_uint<32> *locs_o) {
	ap_uint<32> buf1[MIN_T_1];
	ap_uint<32> buf2[MIN_T_1];
	ap_uint<MIN_T_LOG> c1(0);
	ap_uint<MIN_T_LOG> c2(0);
	ap_uint<OUT_SIZE_LOG> locs_l(0);
LOOP_adjacency_test:
	for (size_t i = 0; i < loc_stra_li; i++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = 70000 // LOCATION_BUFFER_SIZE
		ap_uint<32> loc = loc_stra_i[i];
		if (loc[0]) {
			if (c1 == MIN_T_1) {
				if (loc - buf1[0] < LOC_R) {
					locs_o[locs_l] = (buf1[0].range(31, 1), ap_uint<1>(0));
					locs_l++;
				}
			} else {
				c1++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buf1[j] = buf1[j + 1];
			}
			buf1[MIN_T_1 - 1] = loc;
		} else {
			if (c2 == MIN_T_1) {
				if (loc - buf2[0] < LOC_R) {
					locs_o[locs_l] = buf2[0];
					locs_l++;
				}
			} else {
				c2++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buf2[j] = buf2[j + 1];
			}
			buf2[MIN_T_1 - 1] = loc;
		}
	}
	locs_o[locs_l] = END_LOCATION;
}

void write_locations(const ap_uint<32> *locs_i, ap_uint<32> *locs_o) {
LOOP_write_locs:
	for (size_t i = 0; i < OUT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
		ap_uint<32> loc_buf(locs_i[i]);
		locs_o[i] = loc_buf;
		if (loc_buf == END_LOCATION) break;
	}
}
