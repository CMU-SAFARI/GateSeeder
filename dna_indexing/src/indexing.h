#ifndef INDEXING_H
#define INDEXING_H

#include "extraction.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
	uint32_t n, m; // size of h & location
	uint32_t *h;   // hash table
	uint32_t *loc; // location array
} index_t;

typedef struct {
	min_loc_stra_v *p;
	unsigned int i;
} thread_param_t;

void create_index(FILE *fp, const unsigned int w, const unsigned int k, const unsigned int f, const unsigned int b,
                  index_t *idx);
void parse_extract(FILE *fp, const unsigned int w, const unsigned int k, const unsigned int b, min_loc_stra_v *p);
void sort(min_loc_stra_v *p);
void *thread_merge_sort(void *arg);
void merge_sort(min_loc_stra_t *a, size_t l, size_t r);
void final_merge(min_loc_stra_t *a, size_t n, size_t l, size_t r);
void merge(min_loc_stra_t *a, size_t l, size_t m, size_t r);
#endif
