#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t *h;        // hash table
    uint32_t *position; // position array
    uint8_t *strand;    // strand array
} index_t;

typedef struct {
    uint32_t minimizer;
    uint32_t position;
    uint8_t strand;
} mm72_t;

typedef struct {
    size_t n, m;
    mm72_t *a;
} mm72_v;

index_t *create_index(FILE *fp, const unsigned int w, const unsigned int k);
void merge_sort(mm72_t *a, unsigned int l, unsigned int r);
void merge(mm72_t *a, unsigned int l, unsigned int m, unsigned int r);
#endif
