#ifndef KERNEL_HPP
#define KERNEL_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "ap_int.h"
#pragma GCC diagnostic pop

#include <stdint.h>

#define MAX_IN_LEN 1048576  // 1M
#define MAX_OUT_LEN 1048576 // 1M

// Only for C sim
#define MAX_SEED_LEN 1048576    // 1M
#define MAX_LOC_STR_LEN 1048576 // 1M

#define END_OF_SEQ_BASE 0xe

#define END_OF_READ_SEED (seed_t{0, 0, 0})
#define END_OF_SEQ_SEED (seed_t{0, 1, 0})

#define SEED_BUF_LEN 40
#define SEED_BUF_LEN_LOG 6

#define W 12
#define W_LOG 4

#define K 18

#define READ_LEN_LOG 7

#define MAX_KMER 0xfffffffff
#define MAX_HASH 0xfffffffff

#define H_SIZE 67108864 // b26
#define LS_SIZE 67108864
#define B 26
#define F 1000
#define F_LOG 10
#define MIN_T 3
#define MIN_T_1 (MIN_T - 1)
#define MIN_T_LOG 2
#define LOC_R 150
#define NB_READS 4093747
#define LOCATION_BUFFER_SIZE 40000
#define LOCATION_BUFFER_SIZE_LOG 16
#define OUT_SIZE 5000
#define OUT_SIZE_LOG 13
#define END_LOCATION 0xffffffff

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

#ifdef VARIABLE_LEN
void seeding(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
             const loc_stra_t loc_stra1_m[LS_SIZE], const base_t read_i[IN_SIZE], const ap_uint<32> nb_reads_i,
             loc_stra_t locs0_o[OUT_SIZE], loc_stra_t locs1_o[OUT_SIZE]);

void pipeline(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
              const loc_stra_t loc_stra1_m[LS_SIZE], const base_t *read_i, const ap_uint<32> nb_reads_i,
              loc_stra_t *locs0_o, loc_stra_t *locs1_o);
#else
void kernel(const ap_uint<32> h0_m[H_SIZE], const loc_str_t loc_str0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
            const loc_str_t loc_str1_m[LS_SIZE], const base_t seq_i[MAX_IN_LEN], loc_str_t loc_str0_o[MAX_OUT_LEN],
            loc_str_t loc_str1_o[MAX_OUT_LEN]);

#endif
/*
void pipeline(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
              const loc_stra_t loc_stra1_m[LS_SIZE], const base_t *read_i, loc_stra_t *locs0_o, loc_stra_t *locs1_o);
#endif

void read_read(const base_t *read_i, base_t *read_o);

void get_locations(const min_stra_b_t *p_i, const ap_uint<32> *h_m, const loc_stra_t *loc_stra_m, loc_stra_t *locs_o);
void read_locations(const min_stra_b_t min_stra_i, loc_stra_t *buf_o, ap_uint<F_LOG> &buf_lo, const ap_uint<32> *h_m,
                    const loc_stra_t *loc_stra_m);
void merge_locations(const loc_stra_t *loc_stra_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_li,
                     const loc_stra_t *buf_i, const ap_uint<F_LOG> buf_li, loc_stra_t *loc_stra_o,
                     ap_uint<LOCATION_BUFFER_SIZE_LOG> &loc_stra_lo);

void adjacency_test(const loc_stra_t *loc_stra_i, loc_stra_t *locs_o);
void write_locations(const loc_stra_t *locs_i, loc_stra_t *locs_o);
*/
#endif
