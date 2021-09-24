#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t n, m;      // size of h & position (or strand)
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

typedef struct {
    mm72_v *p;
    unsigned int i;
} thread_param_t;

index_t *create_index(FILE *fp, const unsigned int w, const unsigned int k,
                      const unsigned int filter_threshold,
                      const unsigned int b);
mm72_v *create_raw_index(FILE *fp, const unsigned int w, const unsigned int k,
                         const unsigned int filter_threshold,
                         const unsigned int b);

void parse_sketch(FILE *fp, const unsigned int w, const unsigned int k,
                  const unsigned int b, mm72_v *p);
void sort(mm72_v *p);
void *thread_merge_sort(void *arg);
void merge_sort(mm72_t *a, unsigned int l, unsigned int r);
void final_merge(mm72_t *a, unsigned int n, unsigned int l, unsigned int r);
void merge(mm72_t *a, unsigned int l, unsigned int m, unsigned int r);
#endif
