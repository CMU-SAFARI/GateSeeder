#ifndef KERNEL_HPP
#define KERNEL_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "ap_int.h"
#include <hls_stream.h>
#pragma GCC diagnostic pop

#include <stdint.h>

// NEW //

#define bits_per_base 4

#if bits_per_base == 4
#define base_per_byte 2
#elif bits_per_base == 2
#define base_per_byte 4
#endif

// OLD //

#define MAX_IN_LEN 1048576  // 1M
#define MAX_OUT_LEN 1048576 // 1M

#define END_OF_SEQ_BASE 0xe

#define END_OF_READ_SEED (seed_t{0, 0, 0})
#define END_OF_SEQ_SEED (seed_t{0, 1, 0})

#define SEED_BUF_LEN 400
#define SEED_BUF_LEN_LOG 9

#define W 152
#define W_LOG 8

#define K 19

#define F 2000

#define B 26

#define READ_LEN_LOG 15

#define MAX_KMER 0xfffffffff
#define MAX_HASH 0xfffffffff

#define LOC_STR_FIFO_LEN 80000

#define END_OF_READ_LOC_STR 0xfffffffe
#define END_OF_SEQ_LOC_STR 0xffffffff

#define AFR 20000
#define AFT 9

#define H_SIZE 67108864 // b26
#define LS_SIZE 67108864

#ifdef VARIABLE_LEN
#define MAX_READ_LEN 23000
#define END_OF_READ 0xf
#else
#define READ_LEN 17000
#endif

typedef ap_uint<4> base_t;
typedef ap_uint<32> loc_str_t;
typedef ap_uint<1> str_t;
typedef ap_uint<1> valid_t;

struct pos_t {
	ap_uint<28> start;
	ap_uint<28> end;
	str_t str;
	valid_t valid;
};

struct hash_t {
	ap_uint<2 * K> hash;
	str_t str;
};

struct seed_t {
	ap_uint<B> seed;
	str_t str;
	valid_t valid;
	ap_uint<READ_LEN_LOG> loc;
	int operator==(seed_t x) { return (this->seed == x.seed && this->str == x.str); }
	int operator!=(seed_t x) { return (this->seed != x.seed || this->str != x.str); }
};

void kernel(const uint32_t h0_i[H_SIZE], const uint32_t str_loc0_i[LS_SIZE], const uint32_t h1_i[H_SIZE],
            const uint32_t str_loc1_i[LS_SIZE], const uint32_t seq_i[MAX_IN_LEN], const uint32_t len_i,
            uint32_t str_loc0_o[MAX_OUT_LEN], uint32_t str_loc1_o[MAX_OUT_LEN]);

void read_seq(const uint32_t *seq_i, const uint32_t len_i, hls::stream<base_t> &seq_o);

void get_pos(hls::stream<seed_t> &p_i, const ap_uint<32> *h_m, hls::stream<pos_t> &pos_o);

void get_locations(hls::stream<pos_t> &pos_i, const loc_str_t *loc_str_m, hls::stream<loc_str_t> &loc_str_o);

void read_locations(pos_t pos, const loc_str_t *loc_str_m, hls::stream<loc_str_t> &buf_o);
void merge_locations(hls::stream<loc_str_t> &loc_str_i, hls::stream<loc_str_t> &buf_i,
                     hls::stream<loc_str_t> &loc_str_o);
void adjacency_filtering(hls::stream<loc_str_t> &loc_str_i, hls::stream<loc_str_t> &loc_str_o);

void write_locations(hls::stream<loc_str_t> &loc_str_i, loc_str_t *loc_str_o);

void read_merge_locations(pos_t pos_i, hls::stream<loc_str_t> &buf_i, hls::stream<loc_str_t> &loc_str_i,
                          const loc_str_t *loc_str_m, hls::stream<loc_str_t> &buf_o, hls::stream<loc_str_t> &loc_str_o);
#endif
