#ifndef EXTRACTION_HPP
#define EXTRACTION_HPP

#include "ap_int.h"
#include "hls_stream.h"

#define SE_W 12
#define SE_K 15

typedef struct {
	ap_uint<2 * SE_K> hash; // size: 2*K
	ap_uint<32> loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
} seed_t;

void extract_seeds(const uint8_t *seq_i, const uint32_t nb_bases_i, hls::stream<seed_t> &minimizers_o);

#endif
