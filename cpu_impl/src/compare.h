#ifndef COMPARE_H
#define COMPARE_H

#include "seeding.h"
#include <stdint.h>

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

void compare(target_v tar, read_v reads, cindex_t idx, const size_t len,
             const unsigned int w, const unsigned int k, const unsigned int b,
             const unsigned int min_t, const unsigned int loc_r);
#endif
