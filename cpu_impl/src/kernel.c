#include "kernel.h"
#include "extraction.h"
#include <err.h>
#include <stdlib.h>

#ifdef MULTI_THREAD
#include <pthread.h>

void kernel(const index_t idx, const read_v reads, FILE *fp[NB_THREADS]) {
	pthread_t threads[NB_THREADS];
	thread_param_t params[NB_THREADS];
	for (size_t i = 0; i < NB_THREADS; i++) {
		params[i].idx   = idx;
		params[i].reads = reads;
		params[i].start = i * reads.n / NB_THREADS;
		params[i].end   = (i + 1) * reads.n / NB_THREADS;
		params[i].fp    = fp[i];
		pthread_create(&threads[i], NULL, thread_kernel, (void *)&params[i]);
	}
	for (size_t i = 0; i < NB_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
}

void *thread_kernel(void *arg) {
	thread_param_t *param = (thread_param_t *)arg;
	uint32_t *merge_buf[2];
	merge_buf[0] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	if (merge_buf[0] == NULL) {
		err(1, "malloc");
	}
	merge_buf[1] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	if (merge_buf[1] == NULL) {
		err(1, "malloc");
	}
	loc_str_t *af_buf = (loc_str_t *)malloc(sizeof(loc_str_t) * LOCATION_BUFFER_SIZE);
	if (af_buf == NULL) {
		err(1, "malloc");
	}
	loc_str_v locs;
	locs.a = (loc_str_t *)malloc(LOCATION_BUFFER_SIZE * sizeof(loc_str_t));
	if (locs.a == NULL) {
		err(1, "malloc");
	}
	for (size_t i = param->start; i < param->end; i++) {
#ifdef VARIABLE_LEN
		seeding(param->idx, param->reads.a[i], &locs, param->reads.len[i], merge_buf, af_buf);
#else
		seeding(param->idx, param->reads.a[i], &locs, merge_buf, af_buf);
#endif
		for (size_t j = 0; j < locs.n; j++) {
			char str = locs.a[j].str ? '-' : '+';
			if (j == 0) {
				fprintf(param->fp, "%c.%u", str, locs.a[0].loc);
			} else {
				fprintf(param->fp, "\t%c.%u", str, locs.a[j].loc);
			}
		}
		fputs("\n", param->fp);
	}
	return (void *)NULL;
}

#else

void kernel(const index_t idx, const read_v reads, FILE *fp) {
	uint32_t *merge_buf[2];
	merge_buf[0] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	if (merge_buf[0] == NULL) {
		err(1, "malloc");
	}
	merge_buf[1] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	if (merge_buf[1] == NULL) {
		err(1, "malloc");
	}
	loc_str_t *af_buf = (loc_str_t *)malloc(sizeof(loc_str_t) * LOCATION_BUFFER_SIZE);
	if (af_buf == NULL) {
		err(1, "malloc");
	}
	loc_str_v locs;
	locs.a = (loc_str_t *)malloc(LOCATION_BUFFER_SIZE * sizeof(loc_str_t));
	if (locs.a == NULL) {
		err(1, "malloc");
	}
	for (size_t i = 0; i < reads.n; i++) {
#ifdef VARIABLE_LEN
		seeding(idx, reads.a[i], &locs, reads.len[i], merge_buf, af_buf);
#else
		seeding(idx, reads.a[i], &locs, merge_buf, af_buf);
#endif
		for (size_t j = 0; j < locs.n; j++) {
			char str = locs.a[j].str ? '-' : '+';
			if (j == 0) {
				fprintf(fp, "%c.%u", str, locs.a[0].loc);
			} else {
				fprintf(fp, "\t%c.%u", str, locs.a[j].loc);
			}
		}
		fputs("\n", fp);
	}
}

#endif

#ifdef VARIABLE_LEN
void seeding(const index_t idx, const char *read, loc_str_v *out, size_t len, uint32_t *merge_buf[2],
             loc_str_t *af_buf) {
	seed_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;
	extract_seeds(read, &p, len);
#else
void seeding(const index_t idx, const char *read, loc_str_v *out, uint32_t *merge_buf[2], loc_str_t *af_buf) {
	seed_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;
	extract_seeds(read, &p);
#endif
	size_t merge_buf_len[2] = {0};
	size_t sel              = 0;

	for (size_t i = 0; i < p.n; i++) {
		uint32_t seed          = p.a[i].seed;
		uint32_t start         = (seed == 0) ? 0 : idx.h[seed - 1];
		uint32_t end           = idx.h[seed];
		size_t loc_j           = 0;
		size_t mem_j           = start;
		merge_buf_len[1 - sel] = 0;
		while (loc_j < merge_buf_len[sel] && mem_j < end) {
			uint32_t mem_loc_str = idx.loc_str[mem_j] ^ p.a[i].str;
			if (merge_buf[sel][loc_j] <= mem_loc_str) {
				merge_buf[1 - sel][merge_buf_len[1 - sel]] = merge_buf[sel][loc_j];
				loc_j++;
			} else {
				merge_buf[1 - sel][merge_buf_len[1 - sel]] = mem_loc_str;
				mem_j++;
			}
			merge_buf_len[1 - sel]++;
		}
		while (loc_j < merge_buf_len[sel]) {
			merge_buf[1 - sel][merge_buf_len[1 - sel]] = merge_buf[sel][loc_j];
			loc_j++;
			merge_buf_len[1 - sel]++;
		}
		while (mem_j < end) {
			merge_buf[1 - sel][merge_buf_len[1 - sel]] = idx.loc_str[mem_j] ^ p.a[i].str;
			mem_j++;
			merge_buf_len[1 - sel]++;
		}
		sel = 1 - sel;
	}

#ifdef AF
	size_t n = merge_buf_len[sel];
	out->n   = 0;
	if (n >= AFT) {
		for (size_t i = 0; i < n; i++) {
			af_buf[i].loc = merge_buf[sel][i] & (UINT32_MAX - 1);
			af_buf[i].str = merge_buf[sel][i] & 1;
		}
		size_t loc_counter  = 1;
		size_t loc_offset   = 1;
		size_t init_loc_idx = 0;
		while (init_loc_idx < n - AFT + 1) {
#ifdef VARIABLE_LEN
			if (af_buf[init_loc_idx + loc_offset].loc - af_buf[init_loc_idx].loc < len + len / AFR) {
#else
			if (af_buf[init_loc_idx + loc_offset].loc - af_buf[init_loc_idx].loc <
			    READ_LEN + READ_LEN / AFR) {
#endif
				if (af_buf[init_loc_idx + loc_offset].str == af_buf[init_loc_idx].str) {
					loc_counter++;
					if (loc_counter == AFT) {
						out->a[out->n] = af_buf[init_loc_idx];
						out->n++;
						init_loc_idx++;
						loc_counter = 1;
						loc_offset  = 0;
					}
				}
			} else {
				init_loc_idx++;
				loc_counter = 1;
				loc_offset  = 0;
			}
			loc_offset++;
		}
	}
#else
	out->n = merge_buf_len[sel];
	for (size_t i = 0; i < locs->n; i++) {
		out->a[i].loc = merge_buf[sel][i] & (UINT32_MAX - 1);
		out->a[i].str = merge_buf[sel][i] & 1;
	}
#endif
}
