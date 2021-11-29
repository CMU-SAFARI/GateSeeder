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
#ifdef MULTI_THREAD
#ifndef NB_THREADS
#define NB_THREADS 8
#endif
#endif

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
	FILE *fp;
} thread_param_t;

void read_seeding(const index_t idx, const read_v reads, FILE *fp[NB_THREADS]);
void *thread_read_seeding(void *arg);

#else

void read_seeding(const index_t idx, const read_v reads, FILE *fp);

#endif

void seeding(const index_t idx, const char *read, location_v *locs);
#endif
