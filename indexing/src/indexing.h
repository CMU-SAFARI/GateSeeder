#ifndef INDEXING_H
#define INDEXING_H

#include "types.h"
#include <stdio.h>

index_t gen_index(const target_t target, const unsigned w, const unsigned k, const unsigned b, const unsigned max_occ);
void write_index(FILE *fp, const index_MS_t index, const target_t target, const unsigned w, const unsigned k,
                 const unsigned b, const unsigned max_occ, const size_t MS_size);
index_MS_t partion_index(const index_t index, const size_t MS_size, const unsigned max_nb_MS);
void index_MS_destroy(const index_MS_t index);
void write_gold_index(FILE *fp, const index_t index, const target_t target, const unsigned w, const unsigned k,
                      const unsigned b, const unsigned max_occ);

#endif
