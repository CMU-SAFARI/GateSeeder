#ifndef SEEDING_H
#define SEEDING_H

#include "parse.h"
#include <stdint.h>
#define READ_LEN 100
#define W 12
#define K 18
#define B 26
#define MIN_T 3
#define LOC_R 150

typedef struct {
	size_t n;
	uint32_t *a;
} location_v;

typedef struct {
	uint32_t location;
	uint8_t strand;
} buffer_t;

void seeding(index_t idx, char *read, location_v *locs);
#endif
