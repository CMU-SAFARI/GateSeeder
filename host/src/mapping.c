#include "demeter_util.h"
#include "driver.h"
#include "ksort.h"
#include "mapping.h"
#include <assert.h>
#include <err.h>
#include <pthread.h>

#define sort_key_64(x) (x)
KRADIX_SORT_INIT(64, uint64_t, sort_key_64, 8)

extern unsigned VT_MAX_NB;
extern unsigned VT_DISTANCE;

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
static inline task_t next_task(d_worker_t *const worker, const int no_input) {
	if (worker->input_h == buf_empty && !no_input) {
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
	LOCK(worker->mutex);
	worker->input_h = buf_empty;
	UNLOCK(worker->mutex);
	return 1;
}

typedef struct {
	uint32_t q_start;
	uint32_t q_end;
	int str : 1;
	uint64_t t_start;
	uint64_t t_end;
	uint32_t vt_score;
} vote_t;

typedef struct {
	read_metadata_t metadata;
	vote_t *vote;
	unsigned nb_votes;
} vote_v;

#define TARGET(x) (x >> 23)
#define QUERY(x) ((x >> 1) & ((1 << 22) - 1))
#define STR(x) (x & 1)
#define LOC_OFFSET (1 << 21)

static vote_v vote(uint64_t *const loc, const unsigned len) {
	vote_v v = {.nb_votes = 0};
	// TODO: replace byt mtalloc
	MALLOC(v.vote, vote_t, VT_MAX_NB);
	unsigned counter[2] = {0, 0};

	uint32_t q_start[2] = {0, 0};
	uint32_t q_end[2]   = {0, 0};

	uint64_t t_start[2] = {0, 0};
	uint64_t t_end[2]   = {0, 0};

	uint64_t t_ref[2] = {0, 0};

	for (unsigned i = 0; i < len; i++) {
		const uint64_t cur    = loc[i];
		const int str         = STR(cur);
		const uint64_t target = TARGET(cur);
		const uint32_t query  = QUERY(cur);
		// If curent location in the range, increase the counter and update the values
		if (target <= t_ref[str] + VT_DISTANCE) {
			counter[str]++;
			if (query < q_start[str]) {
				q_start[str] = query;
				t_ref[str]   = target;
			}
			if (query > q_end[str]) {
				q_end[str] = query;
			}
			const uint64_t loc = str ? target - query : target + query - LOC_OFFSET;
			if (loc > t_end[str]) {
				t_end[str] = loc;
			}
			if (loc < t_start[str]) {
				t_start[str] = loc;
			}
		}
		// Else check the number of votes we have and if we are above the coverage threshold
		else {
			if (v.nb_votes == VT_MAX_NB) {
				// If not enough votes, we just continue
				if (v.vote[VT_MAX_NB - 1].vt_score >= counter[str]) {
					const uint64_t loc = str ? target - query : target + query - LOC_OFFSET;
					q_start[str]       = query;
					q_end[str]         = query;
					t_start[str]       = loc;
					t_end[str]         = loc;
					t_ref[str]         = loc;
					counter[str]       = 1;
					continue;
				}
			} else {
				VT_MAX_NB++;
			}
			v.vote[v.nb_votes - 1] = (vote_t){
			    .q_start  = q_start[str],
			    .q_end    = q_end[str],
			    .str      = str,
			    .t_start  = t_start[str],
			    .t_end    = t_end[str],
			    .vt_score = counter[str],
			};
			for (unsigned k = v.nb_votes - 1; k > 0; k--) {
				// TODO
			}
		}
	}
	// TODO

	return v;
}

static void map_seq(uint64_t *const loc, const uint32_t len, const read_metadata_t metadata) {
	(void)metadata;
	radix_sort_64(loc, loc + len);
	/*
	for (unsigned i = 0; i < len; i++) {
	        printf("%lx\n", loc[i]);
	}
	*/
	vote_v v = vote(loc, len);
	if (v.nb_vote != 0) {
		v.metadata = metadata;
	}
}

static void cpu_map(d_worker_t *const worker) {
	uint64_t *const loc   = worker->loc_buf.loc;
	uint64_t *start_ptr   = loc;
	uint32_t len          = 0;
	uint32_t read_counter = 0;
	for (uint32_t i = 0; i < (LB_SIZE >> 3); i++) {
		switch (loc[i]) {
			case (1ULL << 63):
				if (len != 0) {
					map_seq(start_ptr, len, worker->loc_buf.metadata[read_counter]);
				}
				read_counter++;
				start_ptr = &loc[i + 1];
				len       = 0;
				break;
			case (UINT64_MAX):
				if (len != 0) {
					map_seq(start_ptr, len, worker->loc_buf.metadata[read_counter]);
				}
				LOCK(worker->mutex);
				worker->output_h = buf_empty;
				UNLOCK(worker->mutex);
				return;
			default:
				len++;
				break;
		}
	}
	errx(1, "Memory section overflow");
}

// Main routine executed by each thread
static void *mapping_routine(__attribute__((unused)) void *arg) {
	d_worker_t *worker = demeter_get_worker(NULL, 0);
	(void)&worker;
	int no_input = 0;
	while (worker) {
		LOCK(worker->mutex);
		task_t task = next_task(worker, no_input);
		UNLOCK(worker->mutex);
		switch (task) {
			case FILL_INPUT:
				// puts("FILL");
				no_input = fill_input(worker);
				break;
			case TRANSFER_INPUT:
				// puts("INPUT");
				demeter_load_seq(worker);
				break;
			case START_KERNEL:
				// puts("KERNEL");
				//  Since this is done "atomically"
				break;
			case TRANSFER_OUTPUT:
				// puts("OUTPUT_D");
				demeter_load_loc(worker);
				break;
			case HANDLE_OUTPUT:
				// puts("OUTPUT_H");
				cpu_map(worker);
				break;
			case NO_TASK:
				// puts("NO_TASK");
				worker = demeter_get_worker(worker, no_input);
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
