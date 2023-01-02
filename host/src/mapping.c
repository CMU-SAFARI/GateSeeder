#include "demeter_util.h"
#include "driver.h"
#include "mapping.h"
#include <pthread.h>

/*
// Sort, vote and write
static void map_seq(uint64_t *const loc, const uint32_t len, const read_metadata_t metadata) {
        (void)loc;
        (void)len;
        (void)metadata;
        if (len != 0) {
                // Sort

                // Vote

                // Write
        }
}
*/
static pthread_mutex_t parse_fastq_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef enum
{
	FILL_INPUT,
	TRANSFER_INPUT,
	START_KERNEL,
	TRANSFER_OUTPUT,
	HANDLE_OUTPUT,
	NO_TASK,
} task_t;

// Should always been protected by the worker mutex
static inline task_t next_task(d_worker_t *const worker) {
	if (worker->input_h == buf_empty) {
		worker->input_h = buf_transfer;
		return FILL_INPUT;
	}
	if (worker->input_d == buf_empty && worker->input_h == buf_full && demeter_is_complete(worker)) {
		worker->input_d = buf_transfer;
		return TRANSFER_INPUT;
	}
	if (worker->input_d == buf_full && worker->output_d == buf_empty) {
		demeter_start_kernel(worker);
		worker->input_d  = buf_empty;
		worker->output_d = buf_full;
		return START_KERNEL;
	}
	if (worker->output_d == buf_full && worker->output_h == buf_empty && demeter_is_complete(worker)) {
		worker->output_d = buf_transfer;
		return TRANSFER_OUTPUT;
	}
	if (worker->output_h == buf_full) {
		worker->output_h = buf_transfer;
		return HANDLE_OUTPUT;
	}
	return NO_TASK;
}

static int fill_input(d_worker_t *const worker) {
	LOCK(parse_fastq_mutex);
	const int res = fastq_parse(&worker->read_buf);
	UNLOCK(parse_fastq_mutex);
	if (!res || worker->read_buf.len != 0) {
		// printf("read_len[%u] : %u\n", worker->id, worker->read_buf.len);
		LOCK(worker->mutex);
		worker->input_h = buf_full;
		UNLOCK(worker->mutex);
		return 0;
	}
	return 1;
}

static void cpu_map(d_worker_t *const worker) {
	// TODO
	LOCK(worker->mutex);
	worker->output_h = buf_empty;
	UNLOCK(worker->mutex);
}

// Main routine executed by each thread
static void *mapping_routine(__attribute__((unused)) void *arg) {
	d_worker_t *worker = demeter_get_worker(NULL);
	(void)&worker;
	int input_available = 1;
	while (input_available) {
		LOCK(worker->mutex);
		task_t task = next_task(worker);
		UNLOCK(worker->mutex);
		switch (task) {
			case FILL_INPUT:
				input_available = !fill_input(worker);
				break;
			case TRANSFER_INPUT:
				demeter_load_seq(worker);
				break;
			case START_KERNEL:
				// Since this is done "atomically"
				break;
			case TRANSFER_OUTPUT:
				demeter_load_loc(worker);
				break;
			case HANDLE_OUTPUT:
				cpu_map(worker);
				break;
			case NO_TASK:
				worker = demeter_get_worker(worker);
				break;
		}
	}
	return (void *)NULL;
}

void mapping_run(const unsigned nb_threads) {
	pthread_t *threads;
	MALLOC(threads, pthread_t, nb_threads);

	for (unsigned i = 0; i < nb_threads; i++) {
		THREAD_START(threads[i], mapping_routine, NULL);
	}

	for (unsigned i = 0; i < nb_threads; i++) {
		THREAD_JOIN(threads[i]);
	}
	free(threads);
}
