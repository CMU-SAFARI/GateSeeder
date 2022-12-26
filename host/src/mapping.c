#include "demeter_util.h"
#include "mapping.h"
#include <pthread.h>

// Main routine executed by each thread
static void *mapping_routine(__attribute__((unused)) void *arg) { return (void *)NULL; }

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
