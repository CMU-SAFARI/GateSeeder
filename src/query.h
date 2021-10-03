#ifndef QUERY_H
#define QUERY_H

#include "index.h"
#include <stdint.h>
#include <stdio.h>
#define READ_LENGTH 100

typedef struct {
    size_t n;
    uint32_t *location;
} location_v;

typedef struct {
    size_t n;
    char **a;
} read_v;

read_v *parse_fastq(FILE *fp);

// Return the possible locations on the reference genome
location_v *get_locations(index_t *idx, char *read, size_t length);

#endif
