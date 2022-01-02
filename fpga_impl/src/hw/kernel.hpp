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

#define MAX_IN_LEN 1048576  // 1M
#define MAX_OUT_LEN 1048576 // 1M

#define END_OF_SEQ_BASE 0xe

#define END_OF_READ_SEED (seed_t{0, 0, 0})
#define END_OF_SEQ_SEED (seed_t{0, 1, 0})

#define SEED_BUF_LEN 40
#define SEED_BUF_LEN_LOG 6

#define W 12
#define W_LOG 4

#define K 18

#define F 1000

#define B 26

#define READ_LEN_LOG 7

#define MAX_KMER 0xfffffffff
#define MAX_HASH 0xfffffffff

#define LOC_STR_FIFO_LEN 40000

#define END_OF_READ_LOC_STR 0xfffffffe
#define END_OF_SEQ_LOC_STR 0xffffffff

#define AFR 3
#define AFT 120

#define H_SIZE 67108864 // b26
#define LS_SIZE 67108864

#ifdef VARIABLE_LEN
#define MAX_READ_LEN 1000
#define END_OF_READ 0xf
#else
#define READ_LEN 100
#endif

typedef ap_uint<4> base_t;
typedef ap_uint<32> loc_str_t;

struct hash_t {
	ap_uint<2 * K> hash;
	ap_uint<1> str;
};

struct seed_t {
	ap_uint<B> seed;
	ap_uint<1> str;
	ap_uint<1> valid;
	int operator==(seed_t x) { return (this->seed == x.seed && this->str == x.str); }
	int operator!=(seed_t x) { return (this->seed != x.seed || this->str != x.str); }
};

void kernel(const ap_uint<32> h0_m[H_SIZE], const loc_str_t loc_str0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
            const loc_str_t loc_str1_m[LS_SIZE], const base_t seq_i[MAX_IN_LEN], loc_str_t loc_str0_o[MAX_OUT_LEN],
            loc_str_t loc_str1_o[MAX_OUT_LEN]);

void read_seq(const base_t *seq_i, hls::stream<base_t> &seq_o);
void get_locations(hls::stream<seed_t> &p_i, const ap_uint<32> *h_m, const loc_str_t *loc_str_m,
                   hls::stream<loc_str_t> &loc_str_o);

void read_locations(const seed_t seed_i, hls::stream<loc_str_t> &buf_o, const ap_uint<32> *h_m,
                    const loc_str_t *loc_str_m);

void merge_locations(hls::stream<loc_str_t> &loc_str_i, hls::stream<loc_str_t> &buf_i,
                     hls::stream<loc_str_t> &loc_str_o);
void adjacency_filtering(hls::stream<loc_str_t> &loc_str_i, hls::stream<loc_str_t> &loc_str_o);
void write_locations(hls::stream<loc_str_t> &loc_str_i, loc_str_t *loc_str_o);

#endif
