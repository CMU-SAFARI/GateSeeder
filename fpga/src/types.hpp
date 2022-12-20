#ifndef TYPES_HPP
#define TYPES_HPP
#include "ap_int.h"
#include <cstdint>

#define SE_W 12
#define SE_K 15
#define SEQ_LEN 1073741824

// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct seed_t {
	ap_uint<2 * SE_K> hash; // size: 2*K
	ap_uint<32> loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

#define BUCKET_ID_SIZE 26
#define MAP_LEN (1 << BUCKET_ID_SIZE)

const static unsigned bucket_id_msb = BUCKET_ID_SIZE - 1;
const static unsigned seed_id_lsb   = BUCKET_ID_SIZE;
const static unsigned seed_id_msb   = 2 * SE_K - 1;
const static unsigned seed_id_size  = seed_id_msb - bucket_id_msb;

// MAP:
// |--MS ID --|-- MS POS --|
//  31         24         0
#define MS_POS_SIZE 25
#define MS_ID_SIZE (32 - MS_POS_SIZE)
#define KEY_LEN (1 << MS_POS_SIZE)

const static unsigned ms_pos_msb = MS_POS_SIZE - 1;
const static unsigned ms_id_lsb  = MS_POS_SIZE;
// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct ms_pos_t {
	ap_uint<MS_POS_SIZE> start_pos;
	ap_uint<MS_POS_SIZE> end_pos;
	ap_uint<MS_ID_SIZE> ms_id;
	ap_uint<seed_id_size> seed_id;
	uint32_t query_loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

// limited to 1024 chromosomes
// Query loc limited to 2097152
// Target loc limited to 1073741824
// Number of chromosomes limited to 1024
// Total: 64 bits

#define OUT_LEN (1 << 25)
struct loc_t {
	ap_uint<30> target_loc;
	ap_uint<21> query_loc;
	ap_uint<10> chrom_id;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

struct buf_metadata_t {
	uint32_t pos;
	uint32_t len;
	ap_uint<1> EOS;
};

// KEY
// loc -> 32 bits  |
// CH_ID -> 10 bits | 42 bits
#define LOC_SHIFT 42
// STR -> 1 bits
// SEED_ID -> 21 bits
#endif
