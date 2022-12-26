#include "demeter_util.h"
#include "driver.h"
#include "mapping.h"
#include <pthread.h>

// DEBUG
#define MS_SIZE_64 (1ULL << 26)

static void print_results(d_worker_t worker) {
	if (worker.complete) {
		demeter_get_results(worker);
		for (unsigned i = 0; i < MS_SIZE_64; i++) {
			const uint64_t res = worker.loc[i];
			// printf("%x: %lx\n", i, res);
			if (res == UINT64_MAX) {
				return;
			}
		}
		puts("ERROR");
	}
}

// Main routine executed by each thread
static void *mapping_routine(__attribute__((unused)) void *arg) {
	d_worker_t worker = demeter_get_worker();
	print_results(worker);
	while (fastq_parse(&worker.read_buf) == 0) {
		printf("len: %u\n", worker.read_buf.len);
		demeter_host(worker);
		worker = demeter_get_worker();
		print_results(worker);
	}

	if (worker.read_buf.len != 0) {
		printf("len: %u\n", worker.read_buf.len);
		demeter_host(worker);
	} else {
		demeter_release_worker(worker);
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
