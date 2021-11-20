#ifndef EXTRACTION_H
#define EXTRACTION_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint32_t min;
	uint32_t loc;
	uint8_t stra;
} min_loc_stra_t;

typedef struct {
	uint64_t kmer;
	uint32_t loc;
	uint8_t stra;
} kmer_loc_stra_t;

typedef struct {
	size_t n;
	min_loc_stra_t *a;
} min_loc_stra_v;

void extract_minimizers(const char *dna, unsigned int len, unsigned int w, unsigned int k, const unsigned int b,
                        min_loc_stra_v *p, uint32_t offset);

#endif
