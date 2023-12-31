#ifndef DRIVER_H
#define DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "GateSeeder_util.h"

#define LB_SIZE (1ULL << 29)

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

typedef struct {
	uint32_t batch_id;
	uint64_t *loc;
	read_metadata_t *metadata;
} loc_buf_t;

typedef struct {
	unsigned id;
	read_buf_t read_buf;
	loc_buf_t loc_buf;
	volatile buf_state_t input_h;
	volatile buf_state_t input_d;
	volatile buf_state_t output_d;
	volatile buf_state_t output_h;
	pthread_mutex_t mutex;
} d_worker_t;

void demeter_fpga_init(const unsigned nb_cus, const char *const binary_file, const index_t index);
d_worker_t *demeter_get_worker(d_worker_t *const worker, const int no_input);
void demeter_load_seq(d_worker_t *const worker);
void demeter_start_kernel(d_worker_t *const worker);
void demeter_load_loc(d_worker_t *const worker);
int demeter_is_complete(d_worker_t *const worker);
void demeter_fpga_destroy();

#ifdef CPU_EX
void seedfarm_execute(d_worker_t *const worker);
#endif

#ifdef __cplusplus
}
#endif

#endif
