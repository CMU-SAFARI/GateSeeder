#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

typedef struct {
	unsigned nb_sequences;
	uint32_t *len;
	uint8_t **seq;
	char **name;
} target_t;

/*
typedef struct {
        uint64_t loc, hash;
} key128_t;
*/

typedef struct {
	uint32_t loc;
	uint32_t bucket_id;
	uint32_t seed_id;
	uint16_t chrom_id;
	unsigned str : 1;
} keym_t;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	keym_t *keys;
} key_v;

/*
typedef struct {
        uint32_t capacity;
        uint32_t len;
        key128_t *keys;
} key128_v;
*/

typedef struct {
	uint32_t map_len, key_len;
	uint32_t *map;
	keym_t *key;
} index_t;

typedef struct {
	unsigned nb_MS;
	uint32_t *map;
	uint64_t **key;
} index_MS_t;

#endif
