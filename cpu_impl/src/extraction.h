#ifndef EXTRACTION_H
#define EXTRACTION_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t minimizer; // size: b
    uint8_t strand;
} min_stra_t;

typedef struct {
    uint64_t minimizer; // size: 2*k
    uint8_t strand;
} min_stra_reg_t;

typedef struct {
    size_t n;
    min_stra_t a[5000];
} min_stra_v;

void extract_minimizers(const char *read, const size_t len,
                        const unsigned int w, const unsigned int k,
                        const unsigned int b, min_stra_v *p);

void push_min_stra(min_stra_v *p, uint32_t min, uint8_t stra);

#endif
