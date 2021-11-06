#ifndef SEEDING_HPP
#define SEEDING_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wuninitialized"
#include "ap_int.h"
#pragma GCC diagnostic pop

#include <stdint.h>

#define H_SIZE 67108864  // chrom1_w12_k18_f500_b26
#define LS_SIZE 33594136 // chrom1_w12_k18_f500_b26
#define W 12
#define W_LOG 4
#define K 18
#define MAX_KMER 0xfffffffff
#define B 26
#define F 700
#define AVG_LOC 7 // TODO
#define F_LOG 10
#define MIN_T 3
#define MIN_T_1 (MIN_T - 1)
#define MIN_T_LOG 2
#define LOC_R 150
#define READ_LEN 100
#define READ_LEN_LOG 7
#define LOCATION_BUFFER_SIZE 70000  // TODO
#define LOCATION_BUFFER_SIZE_LOG 17 // TODO
#define OUT_SIZE 5000
#define OUT_SIZE_LOG 13
#define MIN_STRA_SIZE 100   // TODO
#define MIN_STRA_SIZE_LOG 7 // TODO

typedef ap_uint<3> base_t;

struct min_stra_t {
	ap_uint<2 * K> minimizer;
	ap_uint<1> strand;
};

struct min_stra_b_t {
	ap_uint<B> minimizer;
	ap_uint<1> strand;
	int operator==(min_stra_b_t x) { return (this->minimizer == x.minimizer && this->strand == x.strand); }
};

void seeding(const ap_uint<32> h[H_SIZE], const ap_uint<32> location[LS_SIZE], const base_t *read_i,
             ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> *locs_len_o);
void get_locations(const min_stra_b_t *p_i, const ap_uint<MIN_STRA_SIZE_LOG> p_li, const ap_uint<32> *h_m,
                   const ap_uint<32> *loc_stra_m, ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_lo);
void read_read(const base_t *read_i, base_t *read_o);
void read_locations(const min_stra_b_t min_stra_i, ap_uint<32> *buf_o, ap_uint<F_LOG> &buf_lo, const ap_uint<32> *h_m,
                    const ap_uint<32> *loc_stra_m);
void merge_locations(const ap_uint<32> *loc_stra_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_li,
                     const ap_uint<32> *buf_i, const ap_uint<F_LOG> buf_li, ap_uint<32> *loc_stra_o,
                     ap_uint<LOCATION_BUFFER_SIZE_LOG> &loc_stra_lo);
void adjacency_test(const ap_uint<32> *loc_stra_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> loc_stra_li,
                    ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_lo);
void write_locations(const ap_uint<32> *locs_i, const ap_uint<OUT_SIZE_LOG> locs_li, ap_uint<32> *locs_o,
                     ap_uint<OUT_SIZE_LOG> *locs_lo);
#endif