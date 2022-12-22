#ifndef EXTRACTION_H
#define EXTRACTION_H

#include "types.h"
#include <stdint.h>

void extraction_init();
uint32_t extract_seeds_mapping(const uint8_t *seq, const uint32_t len, seed_v *const minimizers, int partial);

#endif
