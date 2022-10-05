#ifndef INDEXING_H
#define INDEXING_H

#include "types.h"
#include <stdio.h>

index_t gen_index(const target_t target, const unsigned w, const unsigned k, const unsigned b, const unsigned max_occ);
void write_index(FILE *fp, const index_t index, const target_t target, const unsigned w, const unsigned k,
                 const unsigned b, const unsigned max_occ);
void index_destroy(const index_t index);

#endif
