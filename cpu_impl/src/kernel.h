#ifndef KERNEL_H
#define KERNEL_H

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
#ifndef AFT
#define AFT 10
#endif
#ifndef AFR
#define AFR 10
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
	uint32_t loc;
	uint8_t str;
} loc_str_t;

typedef struct {
	size_t n;
	loc_str_t *a;
} loc_str_v;

#ifdef MULTI_THREAD
typedef struct {
	index_t idx;
	read_v reads;
	size_t start;
	size_t end;
	FILE *fp;
} thread_param_t;

void kernel(const index_t idx, const read_v reads, FILE *fp[NB_THREADS]);
void *thread_kernel(void *arg);

#else

void kernel(const index_t idx, const read_v reads, FILE *fp);

#endif

#ifdef VARIABLE_LEN
void seeding(const index_t idx, const char *read, loc_str_v *out, size_t len, uint32_t *merge_buf[2],
             loc_str_t *af_buf);
#else
void seeding(const index_t idx, const char *read, loc_str_v *out, uint32_t *merge_buf[2], loc_str_t *af_buf);
#endif

#endif
