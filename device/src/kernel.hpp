#ifndef KERNEL_HPP
#define KERNEL_HPP

#include "ap_int.h"
#include "hls_stream.h"

extern "C" void demeter_kernel(const uint32_t nb_bases_i, const uint8_t *seq_i, const uint32_t *map_i,
                               const uint64_t *key_i, uint64_t *out_o);
#endif
