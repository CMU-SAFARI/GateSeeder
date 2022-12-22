#ifndef QUERYING_HPP
#define QUERYING_HPP

#include "hls_stream.h"
#include "types.hpp"

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_o);

void query_index_key(hls::stream<ms_pos_t> &ms_pos_i, const uint64_t *const key_i, hls::stream<uint64_t> &loc_o);

void write_loc(hls::stream<uint64_t> &loc_i, uint64_t *const loc_o);

#endif
