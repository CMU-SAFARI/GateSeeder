#ifndef COMPARE_H
#define COMPARE_H

#include "query.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t start;
    uint32_t end;
    unsigned char quality;
} range_t;

typedef struct {
    size_t n;
    range_t *a;
    char name[100];
} t_location_v;

typedef struct {
    size_t n;
    t_location_v *a;
} target_v;

void parse_paf(FILE *fp, target_v *target);
void compare(target_v tar, read_v reads, index_t idx, const size_t len,
             const unsigned int w, const unsigned int k, const unsigned int b);
#endif
