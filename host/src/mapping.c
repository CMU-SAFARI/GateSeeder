#include "demeter_util.h"
#include "driver.h"
#include "mapping.h"
#include <pthread.h>

static inline void swap_buffers(d_worker_t *const worker, read_buf_t *const read_buf, uint64_t **const loc_buf) {
	const read_buf_t read_buf_tmp = worker->read_buf;
	uint64_t *const loc_buf_tmp   = worker->loc;

	worker->read_buf = *read_buf;
	worker->loc      = *loc_buf;

	*read_buf = read_buf_tmp;
	*loc_buf  = loc_buf_tmp;
}

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

// Main routine executed by each thread
static void *mapping_routine(__attribute__((unused)) void *arg) {
	read_buf_t read_buf;
	read_buf_init(&read_buf, RB_SIZE);
	uint64_t *loc_buf;
	MALLOC(loc_buf, uint64_t, MS_SIZE >> 3);

	//  Itterate until we reach the end of the file
	while (fastq_parse(&read_buf) == 0 || read_buf.len != 0) {
		// Minimize the time during which the worker is inactive
		d_worker_t worker = demeter_get_worker();
		swap_buffers(&worker, &read_buf, &loc_buf);
		demeter_host(worker);
		if (worker.complete) {
			cpu_map(read_buf, loc_buf);
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