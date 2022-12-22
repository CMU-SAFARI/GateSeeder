#ifndef TYPES_HPP
#define TYPES_HPP
#include "ap_int.h"
#include <cstdint>

#define SE_W 12
#define SE_K 15
#define SEQ_LEN 1073741824

#define MAX_READ_SIZE 22
// Query loc limited to 4194304
// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct seed_t {
	ap_uint<2 * SE_K> hash; // size: 2*K
	ap_uint<MAX_READ_SIZE> loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

#define BUCKET_ID_SIZE 27
#define MAP_LEN (1 << BUCKET_ID_SIZE)

const static unsigned bucket_id_msb = BUCKET_ID_SIZE - 1;
const static unsigned seed_id_lsb   = BUCKET_ID_SIZE;
const static unsigned seed_id_msb   = 2 * SE_K - 1;
const static unsigned seed_id_size  = seed_id_msb - bucket_id_msb;

#define KEY_LEN (1 << 26)

// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct ms_pos_t {
	uint32_t start_pos;
	uint32_t end_pos;
	ap_uint<seed_id_size> seed_id;
	ap_uint<MAX_READ_SIZE> query_loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

// limited to 1024 chromosomes
// Target loc limited to 1073741824
// Number of chromosomes limited to 1024
// Total: 64 bits

#define OUT_LEN (1 << 26)
struct loc_t {
	ap_uint<30> target_loc;
	ap_uint<MAX_READ_SIZE> query_loc;
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
