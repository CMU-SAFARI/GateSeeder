#ifndef SEEDING_H
#define SEEDING_H

#include "index.h"
#include <stdint.h>
#define READ_LENGTH 100

typedef struct {
	size_t n;
	uint32_t *a;
} location_v;

typedef struct {
	size_t n;
	char **a;
	char **name;
} read_v;

typedef struct {
	uint32_t location;
	uint8_t strand;
} buffer_t;

void cseeding(cindex_t idx, char *read, const size_t len, const unsigned int w,
              const unsigned int k, const unsigned int b,
              const unsigned int min_t, const unsigned int loc_r,
              location_v *locs);

void seeding(index_t idx, char *read, const size_t len, const unsigned int w,
             const unsigned int k, const unsigned int b,
             const unsigned int min_t, const unsigned int loc_r,
             location_v *locs);
#endif
