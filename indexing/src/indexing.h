#ifndef INDEXING_H
#define INDEXING_H

#include <stdint.h>
void index_gen(const uint32_t w, const uint32_t k, const uint32_t map_size, const uint32_t max_occ,
               const uint32_t ms_size, const char *const target_file_name, const char *const index_file_name,
               const uint32_t nb_threads);
#endif
