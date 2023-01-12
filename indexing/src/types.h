#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

typedef struct {
	uint32_t loc;
	uint32_t bucket_id;
	uint32_t seed_id;
	uint16_t chrom_id;
	int8_t str;
} gkey_t;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	gkey_t *keys;
} gkey_v;

#endif
