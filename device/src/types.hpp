#ifndef TYPES_HPP
#define TYPES_HPP
#include "ap_int.h"
#include "auto_gen.hpp"
#include <cstdint>

#define SEQ_LEN (1 << 25)
#define MAP_LEN (1 << MAP_SIZE)
#define KEY_LEN (1 << 29)
#define LOC_LEN (1 << 26)

#define CHROM_SIZE 30
#define CHROM_ID_SIZE 10
// READ_SIZE 22
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

// limited to 1024 chromosomes
// Target loc limited to 1073741824
// Number of chromosomes limited to 1024
// Total: 64 bits

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
// KEY
// loc -> 32 bits  | (2 last bits useless) [31, 0]
// CH_ID -> 10 bits | 42 bits [41, 32]
// STR -> 1 bits  [42, 42]
// SEED_ID -> 21 bits [63,43]
#endif
