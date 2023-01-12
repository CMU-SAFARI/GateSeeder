#ifndef EXTRACTION_H
#define EXTRACTION_H

#include "types.h"
#include <stdint.h>

void extract_seeds(const uint8_t *seq, const uint32_t len, const uint32_t chrom_id, gkey_v *const minimizers,
                   const uint32_t w, const uint32_t k, const uint32_t size_map);

#endif
