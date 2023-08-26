#include "GateSeeder_util.h"
#include "extraction.h"
#include "querying.h"

static pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;

static volatile uint32_t current_input_seq = 0;
static volatile uint32_t current_input_pos = 0;
static volatile uint32_t current_output    = 0;

typedef struct {
	read_buf_t input;
	index_t index;
} thread_arg_t;

static void *cpu_routine(void *arg) {
	thread_arg_t *thread_arg = (thread_arg_t *)arg;
	const read_buf_t input   = thread_arg->input;
	const index_t index      = thread_arg->index;
	seed_v seeds             = {.capacity = 0, .len = 0, .a = NULL};
	location_v locations     = {.capacity = 0, .len = 0, .a = NULL};

	for (;;) {
		uint32_t len       = 0;
		const uint8_t *seq = NULL;
		LOCK(input_mutex);
		if (current_input_seq != input.metadata_len) {
			seq = &input.seq[current_input_pos];
			len = input.metadata[current_input_seq].len;
			current_input_pos += len + 1;
			current_input_seq++;
		}
		UNLOCK(input_mutex);
		if (seq == NULL) {
			free(seeds.a);
			free(locations.a);
			return (void *)NULL;
		}
		extract_seeds(seq, len, &seeds, 0);
		/*
		fprintf(stdout, "SEEDS:\n");
		for (uint32_t i = 0; i < seeds.len; i++) {
		        printf("S\t%lx\n", seeds.a[i].hash);
		}
		*/
		query_index(seeds, &locations, index);
		/*
		fprintf(stdout, "LOCS:\n");
		for (uint32_t i = 0; i < locations.len; i++) {
		        printf("L\t%lx\n", locations.a[i]);
		}
		*/
	}
}

void cpu_pipeline(const read_buf_t input, const unsigned nb_threads, const index_t index) {
	pthread_t *threads;
	thread_arg_t thread_arg = {.input = input, .index = index};

	current_input_seq = 0;
	current_input_pos = 0;
	current_output    = 0;
	extraction_init();

	PROF_INIT;
	PROF_START;
	MALLOC(threads, pthread_t, nb_threads);
	for (unsigned i = 0; i < nb_threads; i++) {
		THREAD_START(threads[i], cpu_routine, &thread_arg);
	}
	for (unsigned i = 0; i < nb_threads; i++) {
		THREAD_JOIN(threads[i]);
	}
	PROF_END;
	PRINT_PROF("CPU");
	free(threads);
}
