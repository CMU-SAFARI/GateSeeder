#ifndef EXTRACTION_HPP
#define EXTRACTION_HPP

#include "ap_int.h"
#include "hls_stream.h"
#include "types.hpp"

void extract_seeds(const uint8_t *seq_i, const uint32_t nb_bases_i, hls::stream<seed_t> &minimizers_o);

#endif
