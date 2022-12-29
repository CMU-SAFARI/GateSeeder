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

static void fill_input_h(d_worker_t *const worker) {
	fastq_parse(&worker->read_buf);
	LOCK(worker->mutex);
	worker->input_h = buf_loaded;
	UNLOCK(worker->mutex);
}

static void transfer_input(d_worker_t *const worker) {}

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
