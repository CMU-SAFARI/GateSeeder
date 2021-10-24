#ifndef TB_DRIVER_HPP
#define TB_DRIVER_HPP

#include "ap_int.h"
#include <fstream>
#include <stdint.h>
#include <vector>

struct out_loc_t {
    char name[100];
    unsigned int n;
    uint32_t *locs;
};

struct index_t {
    ap_uint<32> *h;
    ap_uint<32> *location;
};

std::vector<out_loc_t> drive_sim(std::ifstream &ifs_read,
                                 std::ifstream &ifs_idx);
index_t parse_index(std::ifstream &ifs);

#endif
