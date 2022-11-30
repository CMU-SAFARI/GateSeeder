#ifndef QUERYING_HPP
#define QUERYING_HPP

#include "hls_stream.h"
#include "types.hpp"

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_0_o,
                     hls::stream<ms_pos_t> &ms_pos_1_o);
#endif