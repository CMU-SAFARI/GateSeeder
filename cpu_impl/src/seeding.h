#ifndef SEEDING_H
#define SEEDING_H

#include "parse.h"
#include <stdint.h>
#include <stdio.h>
#ifndef W
#define W 36
#endif
#ifndef K
#define K 19
#endif
#define B 26
#ifndef ADJACENCY_FILTERING_THRESHOLD
#define ADJACENCY_FILTERING_THRESHOLD 10
#endif
#ifndef ADJACENCY_FILTERING_RANGE
#define ADJACENCY_FILTERING_RANGE 10
#endif
#define LOCATION_BUFFER_SIZE 8000000
#ifdef MULTI_THREAD
#ifndef NB_THREADS
#define NB_THREADS 8
#endif
#endif

#ifndef VARAIBLE_LEN
#ifndef READ_LEN
#define READ_LEN 100
#endif
#endif

typedef struct {
	uint32_t location;
	uint8_t strand;
} loc_stra_t;

typedef struct {
	size_t n;
	loc_stra_t *a;
} loc_stra_v;

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

#ifdef VARIABLE_LEN
void seeding(const index_t idx, const char *read, loc_stra_v *locs, size_t len, uint32_t *location_buffer[2],
             loc_stra_t *loc_stra_buf);
#else
void seeding(const index_t idx, const char *read, loc_stra_v *locs, uint32_t *location_buffer[2],
             loc_stra_t *loc_stra_buf);
#endif

#endif
