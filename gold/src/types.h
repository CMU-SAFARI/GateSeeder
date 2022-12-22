#ifndef TYPES_H
#define TYPES_H

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

#endif
