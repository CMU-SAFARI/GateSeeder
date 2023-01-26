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

extern uint32_t MAX_NB_MAPPING;
extern uint32_t VT_DISTANCE;
extern float VT_FRAC_MAX;
extern float VT_MIN_COV;
extern int VT_EQ;

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
	const int res = fa_parse(&worker->read_buf);
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

static inline int compare_mapping_locations(const uint32_t vt_score_0, const uint32_t q_start_0, const uint32_t q_end_0,
                                            const uint64_t t_start_0, const uint64_t t_end_0, const uint32_t vt_score_1,
                                            const uint32_t q_start_1, const uint32_t q_end_1, const uint64_t t_start_1,
                                            const uint64_t t_end_1) {
	if (vt_score_1 > vt_score_0) {
		return 1;
	} else if (vt_score_0 > vt_score_1) {
		return 0;
	} else {
		const uint32_t q_span_0    = q_end_0 - q_start_0;
		const uint64_t t_span_0    = t_end_0 - t_start_0;
		const uint32_t q_span_1    = q_end_1 - q_start_1;
		const uint64_t t_span_1    = t_end_1 - t_start_1;
		const uint64_t span_diff_0 = (q_span_0 > t_span_0) ? q_span_0 - t_span_0 : t_span_0 - q_span_0;
		const uint64_t span_diff_1 = (q_span_1 > t_span_1) ? q_span_1 - t_span_1 : t_span_1 - q_span_1;
		return span_diff_1 < span_diff_0;
	}
}

static inline uint32_t push_mapping_location(const uint32_t vt_score, const uint32_t q_start, const uint32_t q_end,
                                             const uint64_t t_start, const uint64_t t_end, const int8_t str, record_v r,
                                             const uint32_t vt_cov_threshold) {
	if (q_end - q_start >= vt_cov_threshold) {
		int new_mapping = 0;

		if (r.nb_records == MAX_NB_MAPPING) {
			const record_t rec = r.record[MAX_NB_MAPPING - 1];
			if (compare_mapping_locations(rec.vt_score, rec.q_start, rec.q_end, rec.t_start, rec.t_end,
			                              vt_score, q_start, q_end, t_start, t_end)) {
				new_mapping = 1;
			}
		} else {
			new_mapping = 1;
			r.nb_records++;
		}

		if (new_mapping) {
			r.record[r.nb_records - 1] = (record_t){
			    .q_start  = q_start,
			    .q_end    = q_end,
			    .str      = str,
			    .t_start  = t_start,
			    .t_end    = t_end,
			    .vt_score = vt_score,
			};
			for (uint32_t k = r.nb_records - 1; k > 0; k--) {
				const record_t rec_k   = r.record[k];
				const record_t rec_k_1 = r.record[k - 1];
				if (compare_mapping_locations(rec_k_1.vt_score, rec_k_1.q_start, rec_k_1.q_end,
				                              rec_k_1.t_start, rec_k_1.t_end, rec_k.vt_score,
				                              rec_k.q_start, rec_k.q_end, rec_k.t_start, rec_k.t_end)) {
					r.record[k]     = rec_k_1;
					r.record[k - 1] = rec_k;
				} else {
					break;
				}
			}
		}
	}
	return r.nb_records;
}

#define TARGET(x) (x >> 23)
#define QUERY(x) ((x >> 1) & ((1 << 22) - 1))
#define STR(x) (x & 1)
#define LOC_OFFSET (1 << 21)

static record_v vote(uint64_t *const loc, const uint32_t len, const uint32_t vt_cov_threshold) {
	record_v r = {.nb_records = 0};
	MALLOC(r.record, record_t, MAX_NB_MAPPING);
	uint32_t counter[2] = {0, 0};

	uint32_t q_start[2] = {0, 0};
	uint32_t q_end[2]   = {0, 0};

	uint64_t t_start[2] = {0, 0};
	uint64_t t_end[2]   = {0, 0};

	uint64_t t_ref[2] = {0, 0};
	int8_t str        = 0;

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
		// Else push the mapping location
		else {
			r.nb_records = push_mapping_location(counter[str], q_start[str], q_end[str], t_start[str],
			                                     t_end[str], str, r, vt_cov_threshold);
			q_start[str] = query;
			q_end[str]   = query;
			t_start[str] = loc;
			t_end[str]   = loc;
			t_ref[str]   = target;
			counter[str] = 1;
		}
	}
	r.nb_records = push_mapping_location(counter[str], q_start[str], q_end[str], t_start[str], t_end[str], str, r,
	                                     vt_cov_threshold);

	// Filter based on a fraction of the best voting score
	if (r.nb_records > 1) {
		if (VT_EQ && r.record[0].vt_score == r.record[1].vt_score) {
			r.nb_records = 0;
		} else {
			const uint32_t threshold = (uint32_t)(VT_FRAC_MAX * (float)r.record[0].vt_score);
			for (uint32_t k = 1; k < r.nb_records; k++) {
				if (r.record[k].vt_score < threshold) {
					r.nb_records = k;
					break;
				}
			}
		}
	}
	return r;
}

static void map_seq(uint64_t *const loc, const uint32_t len, const uint32_t batch_id, const read_metadata_t metadata) {
	if (len == 0) {
		record_v r = {.metadata = metadata, .nb_records = 0, .record = NULL};
		paf_write(r, batch_id);
	} else {
		/*
		// Count the number of seeds
		uint32_t nb_seeds = 1;
		uint32_t location = QUERY(loc[0]);
		for (uint32_t i = 1; i < len; i++) {
		        uint32_t new_location = QUERY(loc[i]);
		        if (location != new_location) {
		                location = new_location;
		                nb_seeds++;
		        }
		}
		*/

		radix_sort_64(loc, loc + len);

		/*
		printf("len: %x\n", len);
		for (unsigned i = 0; i < len; i++) {
		        printf("%lx\n", loc[i]);
		}
		*/

		const uint32_t vt_coverage_threshold = (uint32_t)(VT_MIN_COV * (float)metadata.len);
		record_v r                           = vote(loc, len, vt_coverage_threshold);
		r.metadata                           = metadata;
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
