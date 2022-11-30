#include "extraction.hpp"
#include "hls_math.h"
#include "kernel.hpp"
#include <stddef.h>

const int f                = F;
const int loc_str_fifo_len = LOC_STR_FIFO_LEN;
const int aft              = AFT;

#ifdef VARIABLE_LEN
const int max_read_len = MAX_READ_LEN;
#endif

void kernel(const uint32_t h0_i[H_SIZE], const uint32_t str_loc0_i[LS_SIZE], const uint32_t h1_i[H_SIZE],
            const uint32_t str_loc1_i[LS_SIZE], const uint32_t seq_i[MAX_IN_LEN], const uint32_t len_i,
            uint32_t str_loc0_o[MAX_OUT_LEN], uint32_t str_loc1_o[MAX_OUT_LEN]) {
	/*
    #pragma HLS INTERFACE m_axi port = seq_i bundle = seq_i
    #pragma HLS INTERFACE m_axi port = h0_m bundle = h0_m
    #pragma HLS INTERFACE m_axi port = h1_m bundle = h1_m
    #pragma HLS INTERFACE m_axi port = loc_str0_m bundle = loc_str0_m
    #pragma HLS INTERFACE m_axi port = loc_str1_m bundle = loc_str1_m
    #pragma HLS INTERFACE m_axi port = loc_str0_o bundle = loc_str0_o
    #pragma HLS INTERFACE m_axi port = loc_str1_o bundle = loc_str1_o
    */

#pragma HLS dataflow
	hls::stream<base_t> seq;

	hls::stream<seed_t> p0;
	hls::stream<seed_t> p1;

	hls::stream<pos_t> pos0;
	hls::stream<pos_t> pos1;

	hls::stream<loc_str_t> loc_str0;
	hls::stream<loc_str_t> loc_str1;

	hls::stream<loc_str_t> loc_str0_af;
	hls::stream<loc_str_t> loc_str1_af;

#pragma HLS STREAM variable = seq depth = 10

#pragma HLS STREAM variable = p0 depth = 10
#pragma HLS STREAM variable = p1 depth = 10

#pragma HLS STREAM variable = pos0 depth = 2
#pragma HLS STREAM variable = pos1 depth = 2

#pragma HLS STREAM variable = loc_str0 depth = 10
#pragma HLS STREAM variable = loc_str1 depth = 10

#pragma HLS STREAM variable = loc_str0_af depth = 10
#pragma HLS STREAM variable = loc_str1_af depth = 10

#pragma HLS STREAM variable = loc_str0_o depth = 10
#pragma HLS STREAM variable = loc_str1_o depth = 10

	read_seq(seq_i, len_i, seq);

	extract_seeds(seq, p0, p1);

	get_pos(p0, h0_m, pos0);
	get_pos(p1, h1_m, pos1);

	get_locations(pos0, loc_str0_m, loc_str0);
	get_locations(pos1, loc_str1_m, loc_str1);

	adjacency_filtering(loc_str0, loc_str0_af);
	adjacency_filtering(loc_str1, loc_str1_af);

	write_locations(loc_str0_af, loc_str0_o);
	write_locations(loc_str1_af, loc_str1_o);
}

void read_seq(const uint32_t *seq_i, uint32_t len_i, hls::stream<base_t> &seq_o) {
LOOP_read_seq:
	for (size_t i = 0; i < len_i; i++) {
#pragma HLS PIPELINE II = 1
		base_t buf = seq_i[i];
		seq_o << (buf >>);
	}
}

void get_pos(hls::stream<seed_t> &p_i, const ap_uint<32> *h_m, hls::stream<pos_t> &pos_o) {
	seed_t seed_buf = p_i.read();
	if (seed_buf.valid) {
		ap_uint<28> start = seed_buf.seed ? h_m[seed_buf.seed - 1].to_uint() : 0;
		ap_uint<28> end   = h_m[seed_buf.seed];
		pos_o << pos_t{start, end, seed_buf.str, 1};
	} else {
		pos_o << pos_t{0, 0, seed_buf.str, 0};
	}
}

void get_locations(hls::stream<pos_t> &pos_i, const loc_str_t *loc_str_m, hls::stream<loc_str_t> &loc_str_o) {
	hls::stream<loc_str_t> loc_str1;
	hls::stream<loc_str_t> loc_str2;
	hls::stream<loc_str_t> buf1;
	hls::stream<loc_str_t> buf2;
#pragma HLS STREAM variable = loc_str1 depth = loc_str_fifo_len
#pragma HLS STREAM variable = loc_str2 depth = loc_str_fifo_len
#pragma HLS STREAM variable = buf1 depth = f
#pragma HLS STREAM variable = buf2 depth = f

	pos_t pos_buf;
LOOP_get_locations:
	for (;;) {
		// Phase a)
		pos_i >> pos_buf;
		if (pos_buf.valid) {
			read_merge_locations(pos_buf, buf2, loc_str1, loc_str_m, buf1, loc_str2);
		} else {
			if (pos_buf.str) {
				merge_locations(loc_str1, buf2, loc_str_o);
				loc_str_o << END_OF_SEQ_LOC_STR;
				break;
			} else {
				pos_i >> pos_buf;
				if (pos_buf.valid) {
					read_merge_locations(pos_buf, buf2, loc_str1, loc_str_m, buf1, loc_str_o);
				} else {
					merge_locations(loc_str1, buf2, loc_str_o);
					loc_str_o << END_OF_READ_LOC_STR;
					if (pos_buf.str) {
						loc_str_o << END_OF_SEQ_LOC_STR;
						break;
					} else {
						loc_str_o << END_OF_READ_LOC_STR;
					}
				}
			}
		}

		// Phase b)
		pos_i >> pos_buf;
		if (pos_buf.valid) {
			read_merge_locations(pos_buf, buf1, loc_str2, loc_str_m, buf2, loc_str1);
		} else {
			if (pos_buf.str) {
				merge_locations(loc_str2, buf1, loc_str_o);
				loc_str_o << END_OF_SEQ_LOC_STR;
				break;
			} else {
				pos_i >> pos_buf;
				if (pos_buf.valid) {
					read_merge_locations(pos_buf, buf1, loc_str2, loc_str_m, buf2, loc_str_o);
				} else {
					merge_locations(loc_str2, buf1, loc_str_o);
					loc_str_o << END_OF_READ_LOC_STR;
					if (pos_buf.str) {
						loc_str_o << END_OF_SEQ_LOC_STR;
						break;
					} else {
						loc_str_o << END_OF_READ_LOC_STR;
					}
				}
			}
		}
	}
}

void read_merge_locations(pos_t pos_i, hls::stream<loc_str_t> &buf_i, hls::stream<loc_str_t> &loc_str_i,
                          const loc_str_t *loc_str_m, hls::stream<loc_str_t> &buf_o,
                          hls::stream<loc_str_t> &loc_str_o) {
#pragma HLS dataflow
	read_locations(pos_i, loc_str_m, buf_o);
	merge_locations(loc_str_i, buf_i, loc_str_o);
}

void read_locations(pos_t pos_i, const loc_str_t *loc_str_m, hls::stream<loc_str_t> &buf_o) {
LOOP_read_locations:
	for (size_t j = pos_i.start; j < pos_i.end; j++) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = f
		buf_o << (loc_str_m[j] ^ pos_i.str);
	}
}

void merge_locations(hls::stream<loc_str_t> &loc_str_i, hls::stream<loc_str_t> &buf_i,
                     hls::stream<loc_str_t> &loc_str_o) {
	loc_str_t loc_str_reg;
	loc_str_t buf_reg;
	ap_uint<1> loc_str_flag;
	ap_uint<1> buf_flag;
	if (!loc_str_i.empty()) {
		loc_str_i >> loc_str_reg;
		loc_str_flag = 1;
	} else {
		loc_str_flag = 0;
	}

	if (!buf_i.empty()) {
		buf_i >> buf_reg;
		buf_flag = 1;
	} else {
		buf_flag = 0;
	}

LOOP_merge_fisrt_part:
	while (loc_str_flag && buf_flag) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = loc_str_fifo_len
		if (loc_str_reg <= buf_reg) {
			loc_str_o << loc_str_reg;
			if (!loc_str_i.empty()) {
				loc_str_i >> loc_str_reg;
			} else {
				loc_str_flag = 0;
			}
		} else {
			loc_str_o << buf_reg;
			if (!buf_i.empty()) {
				buf_i >> buf_reg;
			} else {
				buf_flag = 0;
			}
		}
	}
	if (loc_str_flag) {
		loc_str_o << loc_str_reg;
	LOOP_merge_second_part:
		while (!loc_str_i.empty()) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = loc_str_fifo_len
			loc_str_o << loc_str_i.read();
		}
	} else {
		loc_str_o << buf_reg;
	LOOP_merge_third_part:
		while (!buf_i.empty()) {
#pragma HLS PIPELINE II        = 1
#pragma HLS loop_tripcount min = 0 max = f
			loc_str_o << buf_i.read();
		}
	}
}

void adjacency_filtering(hls::stream<loc_str_t> &loc_str_i, hls::stream<loc_str_t> &loc_str_o) {
	hls::stream<loc_str_t> buf1;
	hls::stream<loc_str_t> buf2;
#pragma HLS STREAM variable = buf1 depth = aft - 1
#pragma HLS STREAM variable = buf2 depth = aft - 1

	ap_uint<1> state(0);
LOOP_adjacency_filtering:
	for (size_t i = 0; i < location_buffer_size; i++) {
#pragma HLS PIPELINE II = 1
		ap_uint<32> loc_str = loc_str_i[i];
		if (loc_str == END_LOCATION) {
			locs_o[locs_l] = END_LOCATION;
			break;
		} else if (loc_str[0]) {
			if (c1 == MIN_T_1) {
				if (loc_str.range(31, 1) - buf1[0] < LOC_R) {
					locs_o[locs_l] = (ap_uint<1>(0), buf1[0]);
					locs_l++;
				}
			} else {
				c1++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buf1[j] = buf1[j + 1];
			}
			buf1[MIN_T_1 - 1] = loc_str.range(31, 1);
		} else {
			if (c2 == MIN_T_1) {
				if (loc_str.range(31, 1) - buf2[0] < LOC_R) {
					locs_o[locs_l] = (ap_uint<1>(0), buf2[0]);
					locs_l++;
				}
			} else {
				c2++;
			}
			for (size_t j = 0; j < MIN_T_1 - 1; j++) {
				buf2[j] = buf2[j + 1];
			}
			buf2[MIN_T_1 - 1] = loc_str.range(31, 1);
		}
	}
}

void write_locations(hls::stream<loc_str_t> &loc_str_i, loc_str_t *loc_str_o) {
	loc_str_t loc_str_reg;
LOOP_write_locations:
	for (size_t i = 0; i < MAX_OUT_LEN; i++) {
#pragma HLS PIPELINE II = 1
		loc_str_i >> loc_str_reg;
		loc_str_o[i] = loc_str_reg;
		if (loc_str_reg == END_OF_SEQ_LOC_STR) {
			break;
		}
	}
}
