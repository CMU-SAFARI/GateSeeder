#ifndef SORT_H
#define SORT_H

#include "extraction.h"
#include <stddef.h>

typedef struct {
	min_loc_stra_v *p;
	unsigned int i;
} thread_sort_t;

void sort(min_loc_stra_v *p);
void *thread_merge_sort(void *arg);
void merge_sort(min_loc_stra_t *a, size_t l, size_t r);
void final_merge(min_loc_stra_t *a, size_t n, size_t l, size_t r);
void merge(min_loc_stra_t *a, size_t l, size_t m, size_t r);

#endif
