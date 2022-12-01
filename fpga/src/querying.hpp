#ifndef QUERYING_HPP
#define QUERYING_HPP

#include "hls_stream.h"
#include "types.hpp"

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_0_o,
                     hls::stream<ms_pos_t> &ms_pos_1_o);
void query_index_key(hls::stream<ms_pos_t> &ms_pos_0_i, hls::stream<ms_pos_t> &ms_pos_1_i, const uint64_t *key_0_i,
                     const uint64_t *key_1_i, uint64_t *buf_0_i, uint64_t *buf_1_i, hls::stream<loc_t> &location_o);
#endif
