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
#define LOCATION_BUFFER_SIZE 200000
#define NB_THREADS 8

typedef struct {
	size_t n;
	uint32_t *a;
} location_v;

typedef struct {
	uint32_t location;
	uint8_t strand;
} buffer_t;

#ifdef MULTI_THREAD
typedef struct {
	index_t idx;
	read_v reads;
	size_t start;
	size_t end;
} thread_param_t;
#endif

void read_seeding(const index_t idx, const read_v reads);
#ifdef MULTI_THREAD
void *thread_read_seeding(void *arg);
#endif
void seeding(const index_t idx, const char *read, location_v *locs);
#endif
