#ifndef INDEXING_H
#define INDEXING_H

#include <stdint.h>
void index_gen(const uint32_t w, const uint32_t k, const uint32_t size_map, const uint32_t max_occ,
               const uint32_t size_ms, const char *const target_file_name, const char *const index_file_name);
#endif
