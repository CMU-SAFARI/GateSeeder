#ifndef COMPARE_H
#define COMPARE_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t start;
    uint32_t end;
} target_t;

typedef struct {
    size_t n;
    target_t *a;
} target_v;

void parse_paf(FILE *fp, target_v *target);

#endif
