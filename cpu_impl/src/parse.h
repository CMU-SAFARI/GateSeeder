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
#ifdef VARIABLE_LEN
	size_t *len;
#endif
} read_v;

void parse_dat(int fd, exp_loc_v *loc);
void parse_index(int fd, index_t *idx);
void parse_reads(int fd, read_v *reads);

#endif
