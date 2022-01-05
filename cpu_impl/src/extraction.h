#ifndef EXTRACTION_H
#define EXTRACTION_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint32_t seed; // size: b
	uint8_t str;
} seed_t;

typedef struct {
	uint64_t hash; // size: 2*k
	uint8_t str;
} hash_t;

typedef struct {
	size_t n;
	seed_t a[10000]; // TODO
} seed_v;

#ifdef VARIABLE_LEN
void extract_seeds(const char *read, seed_v *p, const size_t len);
#else
void extract_seeds(const char *read, seed_v *p);
#endif
void push_seed(seed_v *p, const hash_t hash);

#endif
