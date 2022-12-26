#ifndef DRIVER_H
#define DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "demeter_util.h"

typedef struct {
	unsigned id;
	read_buf_t read_buf;
	uint64_t *loc;
	volatile unsigned complete : 1;
} d_worker_t;

void demeter_fpga_init(const unsigned nb_cus, const char *const binary_file, const index_t index);
void demeter_host(const d_worker_t worker);
void demeter_get_results(const d_worker_t worker);
d_worker_t demeter_get_worker();
// int demeter_get_results(d_worker_t *worker);
void demeter_release_worker(const d_worker_t worker);
void demeter_fpga_destroy();

#ifdef __cplusplus
}
#endif

#endif
