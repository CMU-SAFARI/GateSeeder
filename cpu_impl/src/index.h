#ifndef INDEX_H
#define INDEX_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
	uint32_t n, m;      // size of h & location
	uint32_t *h;        // hash table
	uint32_t *location; // location array
} index_t;

void read_index(FILE *fp, index_t *idx);
#endif
