#ifndef QUERY_H
#define QUERY_H

#include "index.h"
#include <stdint.h>
#include <stdio.h>
#define READ_LENGTH 100

typedef struct {
    size_t n;
    uint32_t *a;
} location_v;

typedef struct {
    size_t n;
    char **a;
} read_v;

typedef struct {
    uint32_t location;
    uint8_t strand;
} buffer_t;

void parse_fastq(FILE *fp, read_v *reads);

// Return the possible locations on the reference genome
void get_locations(index_t idx, char *read, const size_t len,
                   const unsigned int w, const unsigned int k,
                   const unsigned int b, location_v *locs);

#endif
