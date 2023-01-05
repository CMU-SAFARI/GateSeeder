#ifndef TYPES_HPP
#define TYPES_HPP
#include "ap_int.h"
#include <cstdint>

#define SE_W 10
#define SE_K 15
#define SEQ_LEN (1 << 29)

#define CHROM_SIZE 30
#define CHROM_ID_SIZE 10
#define READ_SIZE 22
// CHROM_SIZE + CHROM_ID_SIZE + READ_SIZE + 2 should be equal to 64
// +2 for str and EOR
// LOC : {EOR, CHROM_ID, TARGET_LOC, QUERY_LOC, STR}

#define LOC_OFFSET (1 << 21)

// Query loc limited to 4194304
// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct seed_t {
	ap_uint<2 * SE_K> hash; // size: 2*K
	ap_uint<READ_SIZE> loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

#define BUCKET_ID_SIZE 30
#define MAP_LEN (1 << BUCKET_ID_SIZE)

const static unsigned bucket_id_msb = BUCKET_ID_SIZE - 1;
const static unsigned seed_id_lsb   = BUCKET_ID_SIZE;
const static unsigned seed_id_msb   = 2 * SE_K - 1;
const static unsigned seed_id_size  = seed_id_msb - bucket_id_msb;

#define KEY_LEN (1 << 29)

// str = 0 and EOR = 1: end of read
// str = 1 and EOR = 1: end of file
struct ms_pos_t {
	uint32_t start_pos;
	uint32_t end_pos;
	// TODO
	// ap_uint<seed_id_size> seed_id;
	ap_uint<READ_SIZE> query_loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

// limited to 1024 chromosomes
// Target loc limited to 1073741824
// Number of chromosomes limited to 1024
// Total: 64 bits

#define LOC_LEN (1 << 26)
struct loc_t {
	ap_uint<CHROM_SIZE> target_loc;
	ap_uint<READ_SIZE> query_loc;
	ap_uint<CHROM_ID_SIZE> chrom_id;
	ap_uint<1> str;
	ap_uint<1> EOR;
};

struct buf_metadata_t {
	uint32_t pos;
	uint32_t len;
	ap_uint<1> EOS;
};

#define KEY_CHROM_ID_START 32
#define KEY_STR_START 42
#define KEY_SEED_ID_START 43
// KEY
// loc -> 32 bits  | (2 last bits useless) [31, 0]
// CH_ID -> 10 bits | 42 bits [41, 32]
// STR -> 1 bits  [42, 42]
// SEED_ID -> 21 bits [63,43]
#endif
