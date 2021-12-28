#include "seeding.hpp"
#include "extraction.hpp"
#include <stddef.h>

const int f                    = F;
const int location_buffer_size = LOCATION_BUFFER_SIZE;
#ifdef VARIABLE_LEN
const int max_read_len = MAX_READ_LEN;
#endif
const int nb_reads = NB_READS;

#ifdef VARIABLE_LEN
void seeding(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
             const loc_stra_t loc_stra1_m[LS_SIZE], const base_t read_i[IN_SIZE], const ap_uint<32> nb_reads_i,
             loc_stra_t locs0_o[OUT_SIZE], loc_stra_t locs1_o[OUT_SIZE]) {
#else
void seeding(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
             const loc_stra_t loc_stra1_m[LS_SIZE], const base_t read_i[IN_SIZE], loc_stra_t locs0_o[OUT_SIZE],
             loc_stra_t locs1_o[OUT_SIZE]) {
#endif
#pragma HLS INTERFACE m_axi port = h0_m bundle = h0_m
#pragma HLS INTERFACE m_axi port = loc_stra0_m bundle = loc_stra0_m
#pragma HLS INTERFACE m_axi port = h1_m bundle = h1_m
#pragma HLS INTERFACE m_axi port = loc_stra1_m bundle = loc_stra1_m
#pragma HLS INTERFACE m_axi port = read_i bundle = read_i
#pragma HLS INTERFACE m_axi port = locs0_o bundle = locs0_o
#pragma HLS INTERFACE m_axi port = locs1_o bundle = locs1_o

#pragma HLS dataflow
#ifdef VARIABLE_LEN
	base_t read[MAX_READ_LEN];
#else
	base_t read[READ_LEN];
#endif
#pragma HLS STREAM variable = read
	read_read(read_i, read);
#ifdef VARIABLE_LEN
	pipeline(h0_m, loc_stra0_m, h1_m, loc_stra1_m, read, nb_reads_i, locs0_o, locs1_o);
#else
	pipeline(h0_m, loc_stra0_m, h1_m, loc_stra1_m, read, locs0_o, locs1_o);
#endif
}

void read_read(const base_t *read_i, base_t *read_o) {
LOOP_read_read:
	for (size_t i = 0; i < IN_SIZE; i++) {
#pragma HLS PIPELINE II = 1
		read_o[i] = read_i[i];
	}
}

#ifdef VARIABLE_LEN
void pipeline(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
              const loc_stra_t loc_stra1_m[LS_SIZE], const base_t *read_i, const ap_uint<32> nb_reads_i,
              loc_stra_t *locs0_o, loc_stra_t *locs1_o) {
LOOP_pipeline:
	for (size_t i = 0; i < nb_reads_i; i++) {
#pragma HLS loop_tripcount min = 0 max = nb_reads
#else
void pipeline(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
              const loc_stra_t loc_stra1_m[LS_SIZE], const base_t *read_i, loc_stra_t *locs0_o, loc_stra_t *locs1_o) {
LOOP_pipeline:
	for (size_t i = 0; i < NB_READS; i++) {
#endif
#pragma HLS dataflow
		min_stra_b_t p0[MIN_STRA_SIZE];
		min_stra_b_t p1[MIN_STRA_SIZE];

		loc_stra_t locs0_gl[LOCATION_BUFFER_SIZE];
		loc_stra_t locs1_gl[LOCATION_BUFFER_SIZE];

		loc_stra_t locs0_at[LOCATION_BUFFER_SIZE];
		loc_stra_t locs1_at[LOCATION_BUFFER_SIZE];

#pragma HLS STREAM variable = p0       // depth = 2
#pragma HLS STREAM variable = p1       // depth = 2
#pragma HLS STREAM variable = locs0_gl // depth = 4
#pragma HLS STREAM variable = locs1_gl // depth = 4
#pragma HLS STREAM variable = locs0_at // depth = 4
#pragma HLS STREAM variable = locs1_at // depth = 4

		extract_minimizers(read_i, p0, p1);

		get_locations(p0, h0_m, loc_stra0_m, locs0_gl);
		get_locations(p1, h1_m, loc_stra1_m, locs1_gl);

		adjacency_test(locs0_gl, locs0_at);
		adjacency_test(locs1_gl, locs1_at);

		write_locations(locs0_at, locs0_o);
		write_locations(locs1_at, locs1_o);
	}
}

void get_locations(const min_stra_b_t *p_i, const ap_uint<32> *h_m, const loc_stra_t *loc_stra_m, loc_stra_t *locs_o) {
	loc_stra_t loc_stra1[LOCATION_BUFFER_SIZE];
	loc_stra_t loc_stra2[LOCATION_BUFFER_SIZE];
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra1_l(0);
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra2_l(0);
	loc_stra_t buf1[F];
	loc_stra_t buf2[F];
	ap_uint<F_LOG> buf1_l;
	ap_uint<F_LOG> buf2_l;
	min_stra_b_t min_stra_buf(p_i[0]);
	if (min_stra_buf.valid == 1) {
		read_locations(min_stra_buf, buf1, buf1_l, h_m, loc_stra_m);
	LOOP_get_locations:
		for (size_t i = 0; i < MIN_STRA_SIZE; i += 2) {
			min_stra_buf = p_i[i + 1];
			if (min_stra_buf.valid == 0) {
				merge_locations(loc_stra1, loc_stra1_l, buf1, buf1_l, locs_o, loc_stra2_l);
				locs_o[loc_stra2_l] = END_LOCATION;
				break;
			}
			merge_locations(loc_stra1, loc_stra1_l, buf1, buf1_l, loc_stra2, loc_stra2_l);
			read_locations(min_stra_buf, buf2, buf2_l, h_m, loc_stra_m);
			min_stra_buf = p_i[i + 2];
			if (min_stra_buf.valid == 0) {
				merge_locations(loc_stra2, loc_stra2_l, buf2, buf2_l, locs_o, loc_stra1_l);
				locs_o[loc_stra1_l] = END_LOCATION;
				break;
			}
			merge_locations(loc_stra2, loc_stra2_l, buf2, buf2_l, loc_stra1, loc_stra1_l);
			read_locations(min_stra_buf, buf1, buf1_l, h_m, loc_stra_m);
		}
	}
}

void read_locations(const min_stra_b_t min_stra_i, loc_stra_t *buf_o, ap_uint<F_LOG> &buf_lo, const ap_uint<32> *h_m,
                    const loc_stra_t *loc_stra_m) {
//#pragma HLS INLINE OFF
	ap_uint<32> minimizer = min_stra_i.minimizer;
	ap_uint<32> min       = minimizer ? h_m[minimizer - 1].to_uint() : 0;
	ap_uint<32> max       = h_m[minimizer];
	buf_lo                = max - min;
LOOP_read_locations:
	for (size_t j = 0; j < buf_lo; j++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = f
		buf_o[j] = loc_stra_m[min + j] ^ min_stra_i.strand;
	}
}

void merge_locations(const loc_stra_t *loc_stra_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_li,
                     const loc_stra_t *buf_i, const ap_uint<F_LOG> buf_li, loc_stra_t *loc_stra_o,
                     ap_uint<LOCATION_BUFFER_SIZE_LOG> &loc_stra_lo) {
	ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_j = 0;
	ap_uint<F_LOG> buf_j                         = 0;
	loc_stra_lo                                  = 0;
LOOP_merge_fisrt_part:
	while (loc_stra_j < loc_stra_li && buf_j < buf_li) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = f
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
#pragma HLS loop_tripcount min = 0 max = f
			loc_stra_o[loc_stra_lo] = buf_i[buf_j];
			loc_stra_lo++;
		}
	} else {
	LOOP_merge_third_part:
		for (; loc_stra_j < loc_stra_li; loc_stra_j++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = location_buffer_size
			loc_stra_o[loc_stra_lo] = loc_stra_i[loc_stra_j];
			loc_stra_lo++;
		}
	}
}

void adjacency_test(const loc_stra_t *loc_stra_i, loc_stra_t *locs_o) {
	ap_uint<31> buf1[MIN_T_1];
	ap_uint<31> buf2[MIN_T_1];
	ap_uint<MIN_T_LOG> c1(0);
	ap_uint<MIN_T_LOG> c2(0);
	ap_uint<OUT_SIZE_LOG> locs_l(0);
LOOP_adjacency_test:
	for (size_t i = 0; i < location_buffer_size; i++) {
#pragma HLS PIPELINE II = 1
		ap_uint<32> loc_stra = loc_stra_i[i];
		if (loc_stra == END_LOCATION) {
			locs_o[locs_l] = END_LOCATION;
			break;
		} else if (loc_stra[0]) {
			if (c1 == MIN_T_1) {
				if (loc_stra.range(31, 1) - buf1[0] < LOC_R) {
					locs_o[locs_l] = (ap_uint<1>(0), buf1[0]);
					locs_l++;
				}
			} else {
				c1++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buf1[j] = buf1[j + 1];
			}
			buf1[MIN_T_1 - 1] = loc_stra.range(31, 1);
		} else {
			if (c2 == MIN_T_1) {
				if (loc_stra.range(31, 1) - buf2[0] < LOC_R) {
					locs_o[locs_l] = (ap_uint<1>(0), buf2[0]);
					locs_l++;
				}
			} else {
				c2++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buf2[j] = buf2[j + 1];
			}
			buf2[MIN_T_1 - 1] = loc_stra.range(31, 1);
		}
	}
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
