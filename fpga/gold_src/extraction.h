#ifndef EXTRACTION_H
#define EXTRACTION_H

#include "parsing.h"
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

void extraction_init();
uint32_t extract_seeds_mapping(const uint8_t *seq, const uint32_t len, seed_v *const minimizers, int partial);

#endif
