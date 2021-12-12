#ifndef SEEDING_HPP
#define SEEDING_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "ap_int.h"
#pragma GCC diagnostic pop

#include <stdint.h>

#define H_SIZE 67108864 // b26
#define LS_SIZE 67108864
#define W 12
#define W_LOG 4
#define K 18
#define MAX_KMER 0xfffffffff
#define B 26
#define F 1000
#define F_LOG 10
#define MIN_T 3
#define MIN_T_1 (MIN_T - 1)
#define MIN_T_LOG 2
#define LOC_R 150
#define IN_SIZE 409374700
#define NB_READS 4093747
#define READ_LEN_LOG 7
#define LOCATION_BUFFER_SIZE 40000
#define LOCATION_BUFFER_SIZE_LOG 16
#define OUT_SIZE 5000
#define OUT_SIZE_LOG 13
#define MIN_STRA_SIZE 40
#define MIN_STRA_SIZE_LOG 6
#define END_MINIMIZER ((min_stra_b_t){0, 0, 0})
#define END_LOCATION 0xffffffff
#ifdef VARIABLE_LEN
#define MAX_READ_LEN 1000
#else
#define READ_LEN 100
#endif

typedef ap_uint<4> base_t;
typedef ap_uint<32> loc_stra_t;

struct min_stra_t {
	ap_uint<2 * K> minimizer;
	ap_uint<1> strand;
};

struct min_stra_b_t {
	ap_uint<B> minimizer;
	ap_uint<1> strand;
	ap_uint<1> valid;
	int operator==(min_stra_b_t x) { return (this->minimizer == x.minimizer && this->strand == x.strand); }
	int operator!=(min_stra_b_t x) { return (this->minimizer != x.minimizer || this->strand != x.strand); }
};

#ifdef VARIABLE_LEN
void seeding(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
             const loc_stra_t loc_stra1_m[LS_SIZE], const base_t read_i[IN_SIZE], const ap_uint<32> nb_reads_i,
             loc_stra_t locs0_o[OUT_SIZE], loc_stra_t locs1_o[OUT_SIZE]);

void pipeline(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
              const loc_stra_t loc_stra1_m[LS_SIZE], const base_t *read_i, const ap_uint<32> nb_reads_i,
              loc_stra_t *locs0_o, loc_stra_t *locs1_o);
#else
void seeding(const ap_uint<32> h0_m[H_SIZE], const loc_stra_t loc_stra0_m[LS_SIZE], const ap_uint<32> h1_m[H_SIZE],
             const loc_stra_t loc_stra1_m[LS_SIZE], const base_t read_i[IN_SIZE], loc_stra_t locs0_o[OUT_SIZE],
             loc_stra_t locs1_o[OUT_SIZE]);

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
#endif
