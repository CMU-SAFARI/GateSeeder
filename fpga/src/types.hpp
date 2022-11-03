#ifndef TYPES_HPP
#define TYPES_HPP
#include "ap_int.h"

#define SE_W 12
#define SE_K 15

// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct seed_t {
	ap_uint<2 * SE_K> hash; // size: 2*K
	ap_uint<32> loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

#define BUCKET_ID_SIZE 26
const static unsigned bucket_id_msb = BUCKET_ID_SIZE - 1;
const static unsigned seed_id_lsb   = BUCKET_ID_SIZE;
const static unsigned seed_id_msb   = 2 * SE_K - 1;
const static unsigned seed_id_size  = seed_id_msb - bucket_id_msb;

// MAP:
// |--MS ID --|-- MS POS --|
//  31         21         0
#define MS_POS_SIZE 22
const static unsigned ms_id_size = 32 - MS_POS_SIZE;
const static unsigned ms_pos_msb = MS_POS_SIZE - 1;
const static unsigned ms_id_lsb  = MS_POS_SIZE;
// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct ms_pos_t {
	ap_uint<MS_POS_SIZE> start_pos;
	ap_uint<MS_POS_SIZE> end_pos;
	ap_uint<seed_id_size> seed_id;
	uint32_t query_loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

struct loc_t {
	uint64_t target;
	uint32_t query;
	ap_uint<1> EOR;
};
#endif
