#ifndef DRIVER_H
#define DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "demeter_util.h"

// TODO: rename LB_SIZE
#define MS_SIZE (1ULL << 29)
#define RB_SIZE (1ULL << 24)

// input_d can only be full or transfer
// after starting the kernel can be empty

// output_d can only be empty or transfer
// after starting the kernel can be full

typedef enum
{
	buf_empty,
	buf_transfer, // used for the d buffers when transfering from/to the host
	buf_full
} buf_state_t;

typedef enum
{
	krnl_busy,
	krnl_running,
	krnl_idle
} krnl_state_t;

typedef struct {
	unsigned id;
	read_buf_t read_buf;
	uint64_t *loc;
	volatile krnl_state_t krnl_state;
	volatile buf_state_t input_h;
	volatile buf_state_t input_d;
	volatile buf_state_t output_d;
	volatile buf_state_t output_h;
	pthread_mutex_t mutex;
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
