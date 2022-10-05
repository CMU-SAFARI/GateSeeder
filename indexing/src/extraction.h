#ifndef EXTRACTION_H
#define EXTRACTION_H

#include "types.h"
#include <stdint.h>

typedef struct {
	uint64_t hash; // size: 2*K
	uint32_t loc;
	unsigned str : 1;
} seed_t;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	seed_t *seeds;
} seed_v;

void extract_seeds(const uint8_t *seq, const uint32_t len, const unsigned chrom_id, key128_v *const minimizers,
                   const unsigned w, const unsigned k);

#endif
