#ifndef QUERYING_H
#define QUERYING_H

#include "demeter_util.h"
#include "extraction.h"
#include <stdint.h>

typedef struct {
	uint32_t capacity;
	uint32_t len;
	uint64_t *a;
} location_v;

void query_index(seed_v seeds, location_v *locations, index_t index);
#endif
