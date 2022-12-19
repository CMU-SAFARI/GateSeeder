#ifndef KERNEL_HPP
#define KERNEL_HPP

#include "ap_int.h"
#include "hls_stream.h"

extern "C" void kernel(const uint32_t nb_bases_i, const uint8_t *seq_i, const uint32_t *map_i, const uint64_t *key_0_i,
                       const uint64_t *key_1_i, uint64_t *out_o);
#endif
