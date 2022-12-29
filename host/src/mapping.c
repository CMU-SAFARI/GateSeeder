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

static void cpu_map(const read_buf_t read_buf, uint64_t *const loc_buf) {
        // printf("len: %u\n", read_buf.len);
        uint32_t start  = 0;
        uint32_t len    = 0;
        uint32_t read_i = 0;
        for (uint32_t i = 0; i < (MS_SIZE >> 3); i++) {
                const uint64_t loc = loc_buf[i];
                if (loc == 1ULL << 63) {
                        map_seq(&loc_buf[start], len, read_buf.metadata[read_i]);
                        start = i + 1;
                        len   = 0;
                        read_i++;
                } else if (loc == UINT64_MAX) {
                        map_seq(&loc_buf[start], len, read_buf.metadata[read_i]);
                        return;
                } else {
                        len++;
                }
        }
        puts("ERROR");
}
*/

static void run_kernel(d_worker_t *const worker) {
	execute(worker);
	int map_seq = 0;
	LOCK(worker->mutex);
	worker->output_d = buf_full;
	worker->input_d  = buf_empty;
	if (worker->output_h == buf_full) {
		worker->output_h = buf_transfer;
		map_seq          = 1;
	}
	UNLOCK(worker->mutex);
	if (map_seq) {
		map_seq(worker);
	}
}

// Here krnl is not running
static void transfer_input(d_worker_t *const worker) {
	load_seq(worker);
	int run_kernel = 0;
	int transfer   = 0;
	int map_seq    = 0;
	LOCK(worker->mutex);
	worker->input_h = buf_empty;
	worker->input_d = buf_full;
	if (worker->output_d == buf_empty) {
		run_kernel = 1;
	} else if (worker->output_d == buf_full && worker->output_h == buf_empty) {
		worker->output_d = buf_transfer;
		transfer         = 1;
	} else if (worker->output_h == buf_full) {
		worker->output_h = buf_transfer;
		map_seq          = 1;
	}
	UNLOCK(worker->mutex);

	if (run_kernel) {
		run_kernel(worker);
	} else if (transfer) {
		transfer_output(worker);
	} else if (map_seq) {
		map_seq(worker);
	}
}

static int fill_input(d_worker_t *const worker) {
	if (fastq_parse(&worker->read_buf) == 0 || worker->read_buf.len != 0) {
		int transfer_input  = 0;
		int transfer_output = 0;
		int map_seq         = 0;

		// TODO: USE THIS FOR THE TRANSITION
		LOCK(worker->mutex);
		worker->input_h = buf_full;

		if (worker->input_d == buf_empty && is_complete(worker)) {
			worker->input_d = buf_transfer;
			transfer_input  = 1;
		} else if (worker_input_d == buf_full && worker_output_d == buf_empty) {
			// TODO: Launch the kernel
			worker->input_d  = buf_empty;
			worker->output_d = buf_full;
		} else if (worker->output_d == buf_full && is_complete(worker)) {
			worker->output_d = buf_transfer;
			transfer_output  = 1;
		} else if (worker->ouput_h == buf_full) {
			worker->output_h = buf_transfer;
			map_seq          = 1;
		}
		UNLOCK(worker->mutex);
		if (transfer) {
			transfer_input(worker);
		}
		return 0;
	}
	return 1;
}

// Main routine executed by each thread
// Implement the thread state machine
static void *mapping_routine(__attribute__((unused)) void *arg) {
	d_worker_t worker;
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
