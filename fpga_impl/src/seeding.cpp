#include "seeding.hpp"
#include "extraction.hpp"
#include <stddef.h>

void seeding(const ap_uint<32> h_m[H_SIZE], const ap_uint<32> loc_stra_m[LS_SIZE], const base_t read_i[READ_LEN],
             ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_lo) {
#pragma HLS INTERFACE mode = m_axi port = h_m bundle = h_m
#pragma HLS INTERFACE mode = m_axi port = loc_stra_m bundle = loc_stra_m
#pragma HLS INTERFACE mode = m_axi port = read_i bundle = read_i
#pragma HLS INTERFACE mode = ap_hs port = read_i
#pragma HLS INTERFACE mode = m_axi port = locs_o depth = 5000 bundle = locs_o // OUT_SIZE TODO
//#pragma HLS INTERFACE mode = ap_ovld port = locs_o                            // OUT_SIZE TODO
#pragma HLS INTERFACE mode = s_axilite port = locs_lo
//#pragma HLS INTERFACE mode = ap_ovld port = locs_lo

#pragma HLS dataflow
	base_t read[READ_LEN];
	min_stra_b_t p[MIN_STRA_SIZE];
	ap_uint<MIN_STRA_SIZE_LOG> p_l;
	ap_uint<32> locs[OUT_SIZE];
	ap_uint<OUT_SIZE_LOG> locs_l;
#pragma HLS STREAM variable        = read
#pragma HLS ARRAY_RESHAPE variable = p type = complete dim = 1
#pragma HLS STREAM variable = p type = fifo depth = 2
#pragma HLS STREAM variable                       = locs
        read_read(read_i, read);
        extract_minimizers(read, p, p_l);
        get_locations(p, p_l, h_m, loc_stra_m, locs, locs_l);
        write_locations(locs, locs_l, locs_o, locs_lo);
}

void read_read(const base_t *read_i, base_t *read_o) {
LOOP_read_read:
	for (size_t i = 0; i < READ_LEN; i++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 100 max = 100 // READ_LEN
		read_o[i] = read_i[i];
	}
}

void get_locations(const min_stra_b_t *p_i, const ap_uint<MIN_STRA_SIZE_LOG> p_li, const ap_uint<32> *h_m,
                   const ap_uint<32> *loc_stra_m, ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_lo) {
	ap_uint<32> loc_stra1[LOCATION_BUFFER_SIZE];
	ap_uint<32> loc_stra2[LOCATION_BUFFER_SIZE];
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra1_l(0);
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra2_l(0);
	ap_uint<32> buf1[F];
	ap_uint<32> buf2[F];
	ap_uint<F_LOG> buf1_l;
	ap_uint<F_LOG> buf2_l;
LOOP_get_locations:
	for (size_t i = 0; i < p_li / 2; i++) {
#pragma HLS loop_tripcount min = 0 max = 50 // READ_LEN / 2
		read_locations(p_i[2 * i], buf1, buf1_l, h_m, loc_stra_m);
		merge_locations(loc_stra1, loc_stra1_l, buf1, buf1_l, loc_stra2, loc_stra2_l);
		read_locations(p_i[2 * i + 1], buf2, buf2_l, h_m, loc_stra_m);
		merge_locations(loc_stra2, loc_stra2_l, buf2, buf2_l, loc_stra1, loc_stra1_l);
	}
	if (p_li % 2) {
		read_locations(p_i[p_li - 1], buf1, buf1_l, h_m, loc_stra_m);
		merge_locations(loc_stra1, loc_stra1_l, buf1, buf1_l, loc_stra2, loc_stra2_l);
		adjacency_test(loc_stra2, loc_stra2_l, locs_o, locs_lo);
	} else {
		adjacency_test(loc_stra1, loc_stra1_l, locs_o, locs_lo);
	}
}

void read_locations(const min_stra_b_t min_stra_i, ap_uint<32> *buf_o, ap_uint<F_LOG> &buf_lo, const ap_uint<32> *h_m,
                    const ap_uint<32> *loc_stra_m) {
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
                    ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_lo) {
	ap_uint<32> buf1[MIN_T_1];
	ap_uint<32> buf2[MIN_T_1];
	ap_uint<MIN_T_LOG> c1(0);
	ap_uint<MIN_T_LOG> c2(0);
	locs_lo = 0;
LOOP_adjacency_test:
	for (size_t i = 0; i < loc_stra_li; i++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = 70000 // LOCATION_BUFFER_SIZE
		ap_uint<32> loc = loc_stra_i[i];
		if (loc[0]) {
			if (c1 == MIN_T_1) {
				if (loc - buf1[0] < LOC_R) {
					locs_o[locs_lo] = (buf1[0].range(31, 1), ap_uint<1>(0));
					locs_lo++;
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
					locs_o[locs_lo] = buf2[0];
					locs_lo++;
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
}

void write_locations(const ap_uint<32> *locs_i, const ap_uint<OUT_SIZE_LOG> locs_li, ap_uint<32> *locs_o,
                     ap_uint<OUT_SIZE_LOG> &locs_lo) {
LOOP_write_locs:
	for (size_t i = 0; i < locs_li; i++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = 5000 // OUT_SIZE
		locs_o[i] = locs_i[i];
	}
	locs_lo = locs_li;
}
