#ifndef DRIVER_H
#define DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "demeter_util.h"

typedef struct {
	unsigned id;
	uint8_t *seq;
	uint64_t *loc;
	uint32_t len;
	volatile unsigned complete : 1;
} d_worker_t;

void demeter_fpga_init(const unsigned nb_kernels, const unsigned nb_ms_key, const char *const binary_file,
                       const index_t index);
void demeter_host(const d_worker_t worker);
d_worker_t demeter_get_worker();
int demeter_get_results(d_worker_t *worker);
void demeter_fpga_destroy();

#ifdef __cplusplus
}
#endif

#endif
