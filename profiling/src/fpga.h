#ifndef FPGA_H
#define FPGA_H
#include "GateSeeder_util.h"

#ifdef __cplusplus
extern "C" {
#endif

void fpga_init(const unsigned nb_cus, const uint32_t batch_capacity, const char *const binary_file,
               const index_t index);
int fpga_pipeline();
void fpga_destroy();

#ifdef __cplusplus
}
#endif

#endif
