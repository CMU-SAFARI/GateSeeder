#ifndef EXTRACTION_H
#define EXTRACTION_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint32_t minimizer; // size: b
	uint8_t strand;
} min_stra_t;

typedef struct {
	uint64_t minimizer; // size: 2*k
	uint8_t strand;
} min_stra_reg_t;

typedef struct {
	size_t n;
	min_stra_t a[10000]; // TODO
} min_stra_v;

#ifdef VARIABLE_LEN
void extract_minimizers(const char *read, min_stra_v *p, size_t len);
#else
void extract_minimizers(const char *read, min_stra_v *p);
#endif
void push_min_stra(min_stra_v *p, uint64_t min, uint8_t stra);

#endif
