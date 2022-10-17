#ifndef KERNEL_HPP
#define KERNEL_HPP

#include "ap_int.h"
#include "hls_stream.h"

#define SE_K 15

typedef struct {
	ap_uint<2 * SE_K> hash; // size: 2*K
	ap_uint<32> loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
} seed_t;

#endif
