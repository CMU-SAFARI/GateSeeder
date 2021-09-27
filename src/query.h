#ifndef QUERY_H
#define QUERY_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t n;
    uint32_t *location;
} location_v;

void querry(index_t *idx, FILE *fp);
location_v *get_locations(index_t *idx, char *read, size_t length);

#endif
