#ifndef INDEXING_H
#define INDEXING_H

#include "extraction.h"
#include <stdint.h>

typedef struct {
	uint32_t n, m; // size of h & location
	uint32_t *h;   // hash table
	uint32_t *loc; // location array
} index_t;

typedef struct {
	min_loc_stra_v p;
	size_t id;
	const char *name;
	unsigned int f;
	unsigned int b;
} thread_index_t;

typedef struct {
	size_t filter_counter;
	size_t nempty_counter;
	size_t n;
	size_t m;
	float sd;
	size_t offset;
} info_t;

void create_index(int fd, const unsigned int w, const unsigned int k, const unsigned int f, const unsigned int b,
                  const char *name);
void create_index_part(int fd, const unsigned int w, const unsigned int k, const unsigned int f, const unsigned int b,
                       const unsigned int t, const char *name);
void *thread_create_index(void *arg);
info_t build_index(min_loc_stra_v p, const unsigned int f, const unsigned int b, index_t *idx, const uint32_t offset,
                   char comp);
void parse_extract(int fd, const unsigned int w, const unsigned int k, const unsigned int b, min_loc_stra_v *p);
void write_info(const char *name, const size_t nb);
#endif
