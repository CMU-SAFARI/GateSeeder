#ifndef SEEDING_HPP
#define SEEDING_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
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
#define MIN_T_1 2
#define MIN_T_LOG 2
#define LOC_R 150
#define READ_LEN 100
#define READ_LEN_LOG 7
#define LOCATION_BUFFER_SIZE 70000  // TODO
#define LOCATION_BUFFER_SIZE_LOG 17 // TODO
#define OUT_SIZE 5000
#define OUT_SIZE_LOG 13
#define MIN_STRA_SIZE 5000   // TODO
#define MIN_STRA_SIZE_LOG 13 // TODO

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

struct min_stra_v {
	ap_uint<MIN_STRA_SIZE_LOG> n;
	min_stra_b_t a[MIN_STRA_SIZE];
	// min_stra_v() { this->n = 0; }
};

void seeding(const ap_uint<32> h[H_SIZE], const ap_uint<32> location[LS_SIZE], const base_t *read_i,
             ap_uint<32> *locs_o, ap_uint<OUT_SIZE_LOG> &locs_len_o);
void read_read(base_t *read_buff, const base_t *read_i);
void get_locations(const min_stra_b_t min_stra, ap_uint<32> *mem_buffer, ap_uint<F_LOG> &len, const ap_uint<32> *h,
                   const ap_uint<32> *location);
void merge_locations(const ap_uint<32> *buffer_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer_i_len,
                     ap_uint<32> *buffer_o, ap_uint<LOCATION_BUFFER_SIZE_LOG> &buffer_o_len,
                     const ap_uint<32> *mem_buffer, const ap_uint<F_LOG> mem_buffer_len);
void inline query_locations(const min_stra_b_t min_stra, ap_uint<32> *mem_buffer_w, ap_uint<F_LOG> &mem_buffer_len_w,
                            const ap_uint<32> *h, const ap_uint<32> *location, const ap_uint<32> *buffer_i,
                            const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer_i_len, ap_uint<32> *buffer_o,
                            ap_uint<LOCATION_BUFFER_SIZE_LOG> &buffer_o_len, const ap_uint<32> *mem_buffer_r,
                            const ap_uint<F_LOG> mem_buffer_len_r);

void inline loop_query_locations(const min_stra_v p, ap_uint<32> *location_buffer1, ap_uint<32> *location_buffer2,
                                 ap_uint<LOCATION_BUFFER_SIZE_LOG> &location_buffer1_len,
                                 ap_uint<LOCATION_BUFFER_SIZE_LOG> &location_buffer2_len, ap_uint<1> &buffer_sel,
                                 const ap_uint<32> *h, const ap_uint<32> *location);
void adjacency_test(const ap_uint<32> *buffer1_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer1_i_len,
                    const ap_uint<32> *buffer2_i, const ap_uint<LOCATION_BUFFER_SIZE_LOG> buffer2_i_len,
                    const ap_uint<1> buffer_sel, ap_uint<32> *buffer_o, ap_uint<OUT_SIZE_LOG> &buffer_o_len);

void write_locs(ap_uint<32> *locs_o, const ap_uint<32> *locs_buffer, const ap_uint<OUT_SIZE_LOG> locs_len,
                ap_uint<OUT_SIZE_LOG> &locs_len_o);
#endif
