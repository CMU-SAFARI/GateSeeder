#include "demeter_util.h"
#include "driver.h"
#include "formating.h"
#include "ksort.h"
#include "mapping.h"
#include <assert.h>
#include <err.h>
#include <pthread.h>

#define sort_key_64(x) (x)
KRADIX_SORT_INIT(64, uint64_t, sort_key_64, 8)

extern unsigned MAX_NB_MAPPING;
extern unsigned VT_DISTANCE;

static pthread_mutex_t fastq_parse_mutex = PTHREAD_MUTEX_INITIALIZER;

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
	LOCK(fastq_parse_mutex);
	const int res = fastq_parse(&worker->read_buf);
	UNLOCK(fastq_parse_mutex);
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

#define TARGET(x) (x >> 23)
#define QUERY(x) ((x >> 1) & ((1 << 22) - 1))
#define STR(x) (x & 1)
#define LOC_OFFSET (1 << 21)

static record_v vote(uint64_t *const loc, const unsigned len) {
	record_v r = {.nb_records = 0};
	// TODO: replace byt mtalloc
	MALLOC(r.record, record_t, MAX_NB_MAPPING);
	unsigned counter[2] = {0, 0};

	uint32_t q_start[2] = {0, 0};
	uint32_t q_end[2]   = {0, 0};

	uint64_t t_start[2] = {0, 0};
	uint64_t t_end[2]   = {0, 0};

	uint64_t t_ref[2] = {0, 0};
	unsigned str      = 0;

	for (unsigned i = 0; i < len; i++) {
		const uint64_t cur    = loc[i];
		str                   = STR(cur);
		const uint64_t target = TARGET(cur);
		const uint32_t query  = QUERY(cur);
		const uint64_t loc    = str ? target - query : target + query - LOC_OFFSET;
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
			if (loc > t_end[str]) {
				t_end[str] = loc;
			}
			if (loc < t_start[str]) {
				t_start[str] = loc;
			}
		}
		// Else check the number of votes we have and if we are above the coverage threshold
		else {
			if (r.nb_records == MAX_NB_MAPPING) {
				// If not enough votes, we just continue
				if (r.record[MAX_NB_MAPPING - 1].vt_score >= counter[str]) {
					q_start[str] = query;
					q_end[str]   = query;
					t_start[str] = loc;
					t_end[str]   = loc;
					t_ref[str]   = target;
					counter[str] = 1;
					continue;
				}
			} else {
				r.nb_records++;
			}
			r.record[r.nb_records - 1] = (record_t){
			    .q_start  = q_start[str],
			    .q_end    = q_end[str],
			    .str      = str,
			    .t_start  = t_start[str],
			    .t_end    = t_end[str],
			    .vt_score = counter[str],
			};
			for (unsigned k = r.nb_records - 1; k > 0; k--) {
				if (r.record[k].vt_score > r.record[k - 1].vt_score) {
					const record_t tmp = r.record[k];
					r.record[k]        = r.record[k - 1];
					r.record[k - 1]    = tmp;
				} else {
					break;
				}
			}
			q_start[str] = query;
			q_end[str]   = query;
			t_start[str] = loc;
			t_end[str]   = loc;
			t_ref[str]   = target;
			counter[str] = 1;
		}
	}

	if (r.nb_records == MAX_NB_MAPPING) {
		if (r.record[MAX_NB_MAPPING - 1].vt_score >= counter[str]) {
			return r;
		}
	} else {
		r.nb_records++;
	}
	r.record[r.nb_records - 1] = (record_t){
	    .q_start  = q_start[str],
	    .q_end    = q_end[str],
	    .str      = str,
	    .t_start  = t_start[str],
	    .t_end    = t_end[str],
	    .vt_score = counter[str],
	};
	for (unsigned k = r.nb_records - 1; k > 0; k--) {
		if (r.record[k].vt_score > r.record[k - 1].vt_score) {
			const record_t tmp = r.record[k];
			r.record[k]        = r.record[k - 1];
			r.record[k - 1]    = tmp;
		} else {
			break;
		}
	}
	return r;
}

static void map_seq(uint64_t *const loc, const uint32_t len, const uint32_t batch_id, const read_metadata_t metadata) {
	if (len == 0) {
		record_v r = {.metadata = metadata, .nb_records = 0, .record = NULL};
		paf_write(r, batch_id);
	} else {
		radix_sort_64(loc, loc + len);
		/*
		for (unsigned i = 0; i < len; i++) {
		        printf("%lx\n", loc[i]);
		}
		*/
		record_v r = vote(loc, len);
		r.metadata = metadata;
		/*
		for (unsigned i = 0; i < v.nb_votes; i++) {
		        printf("%lx: nb_votes: %x\n", v.vote[i].t_start, v.vote[i].vt_score);
		}
		*/
		paf_write(r, batch_id);
	}
}

static void cpu_map(d_worker_t *const worker) {
	uint64_t *const loc             = worker->loc_buf.loc;
	uint64_t *start_ptr             = loc;
	uint32_t len                    = 0;
	uint32_t read_counter           = 0;
	const uint32_t batch_id         = worker->loc_buf.batch_id;
	read_metadata_t *const metadata = worker->loc_buf.metadata;
	for (uint32_t i = 0; i < (LB_SIZE >> 3); i++) {
		switch (loc[i]) {
			case (1ULL << 63):
				map_seq(start_ptr, len, batch_id, metadata[read_counter]);
				read_counter++;
				start_ptr = &loc[i + 1];
				len       = 0;
				break;
			case (UINT64_MAX):
				flockfile(stdout);
				// printf("counter: %u\n", read_counter);
				funlockfile(stdout);
				free(worker->loc_buf.metadata);
				LOCK(worker->mutex);
				worker->output_h = buf_empty;
				UNLOCK(worker->mutex);
				paf_batch_set_full(batch_id);
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
