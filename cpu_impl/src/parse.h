#ifndef PARSE_H
#define PARSE_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
	size_t n;
	uint32_t *loc;
} loc_v;

typedef struct {
	size_t n;
	loc_v *loc;
} exp_loc_v;

typedef struct {
	uint32_t n, m;      // size of h & location
	uint32_t *h;        // hash table
	uint32_t *location; // location array
} index_t;

typedef struct {
	size_t n;
	char **a;
	char **name;
} read_v;

void parse_dat(FILE *fp, exp_loc_v *loc);
void parse_index(FILE *fp, index_t *idx);
void parse_fastq(FILE *fp, read_v *reads);

#endif
