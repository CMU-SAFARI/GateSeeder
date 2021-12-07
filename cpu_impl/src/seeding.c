#include "seeding.h"
#include "extraction.h"
#include <stdlib.h>

#ifdef MULTI_THREAD
#include <pthread.h>

void read_seeding(const index_t idx, const read_v reads, FILE *fp[NB_THREADS]) {
	pthread_t threads[NB_THREADS];
	thread_param_t params[NB_THREADS];
	for (size_t i = 0; i < NB_THREADS; i++) {
		params[i].idx   = idx;
		params[i].reads = reads;
		params[i].start = i * reads.n / NB_THREADS;
		params[i].end   = (i + 1) * reads.n / NB_THREADS;
		params[i].fp    = fp[i];
		pthread_create(&threads[i], NULL, thread_read_seeding, (void *)&params[i]);
	}
	for (size_t i = 0; i < NB_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
}

void *thread_read_seeding(void *arg) {
	thread_param_t *param = (thread_param_t *)arg;
	for (size_t i = param->start; i < param->end; i++) {
		location_v locs;
#ifdef VARIABLE_LEN
		seeding(param->idx, param->reads.a[i], &locs, size);
#else
		seeding(param->idx, param->reads.a[i], &locs);
#endif
		for (size_t j = 0; j < locs.n; j++) {
			if (j == 0) {
				fprintf(param->fp, "%u", locs.a[0]);
			} else {
				fprintf(param->fp, "\t%u", locs.a[j]);
			}
		}
		fputs("\n", param->fp);
	}
	return (void *)NULL;
}

#else

void read_seeding(const index_t idx, const read_v reads, FILE *fp) {
	for (size_t i = 0; i < reads.n; i++) {
		location_v locs;
#ifdef VARIABLE_LEN
		seeding(param->idx, param->reads.a[i], &locs, size);
#else
		seeding(param->idx, param->reads.a[i], &locs);
#endif
		for (size_t j = 0; j < locs.n; j++) {
			if (j == 0) {
				fprintf(fp, "%u", locs.a[0]);
			} else {
				fprintf(fp, "\t%u", locs.a[j]);
			}
		}
		fputs("\n", fp);
	}
}

#endif

#ifdef VARIABLE_LEN
void seeding(const index_t idx, const char *read, location_v *locs, size_t size) {
	min_stra_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;
	extract_minimizers(read, &p, size);
#else
void seeding(const index_t idx, const char *read, location_v *locs) {
	min_stra_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;
	extract_minimizers(read, &p);
#endif
	uint32_t location_buffer[2][LOCATION_BUFFER_SIZE]; // Buffers which stores
	                                                   // the locations and the
	                                                   // corresponding strand
	size_t location_buffer_len[2] = {0};
	size_t sel                    = 0;

	for (size_t i = 0; i < p.n; i++) {
		uint32_t minimizer           = p.a[i].minimizer;
		uint32_t min                 = (minimizer == 0) ? 0 : idx.h[minimizer - 1];
		uint32_t max                 = idx.h[minimizer];
		size_t loc_j                 = 0;
		size_t mem_j                 = min;
		location_buffer_len[1 - sel] = 0;
		while (loc_j < location_buffer_len[sel] && mem_j < max) {
			uint32_t mem_location = idx.location[mem_j] ^ p.a[i].strand;
			if (location_buffer[sel][loc_j] <= mem_location) {
				location_buffer[1 - sel][location_buffer_len[1 - sel]] = location_buffer[sel][loc_j];
				loc_j++;
			} else {
				location_buffer[1 - sel][location_buffer_len[1 - sel]] = mem_location;
				mem_j++;
			}
			location_buffer_len[1 - sel]++;
		}
		while (loc_j < location_buffer_len[sel]) {
			location_buffer[1 - sel][location_buffer_len[1 - sel]] = location_buffer[sel][loc_j];
			loc_j++;
			location_buffer_len[1 - sel]++;
		}
		while (mem_j < max) {
			location_buffer[1 - sel][location_buffer_len[1 - sel]] = idx.location[mem_j] ^ p.a[i].strand;
			mem_j++;
			location_buffer_len[1 - sel]++;
		}
		sel = 1 - sel;
	}

	// Adjacency test
	size_t n = location_buffer_len[sel];
	locs->n  = 0;
	if (n >= 3) {
		buffer_t buffer[LOCATION_BUFFER_SIZE];
		for (size_t i = 0; i < n; i++) {
			buffer[i].location = location_buffer[sel][i] & (UINT32_MAX - 1);
			buffer[i].strand   = location_buffer[sel][i] & 1;
		}
		uint32_t loc_buffer[LOCATION_BUFFER_SIZE];
		size_t loc_counter  = 1;
		size_t loc_offset   = 1;
		size_t init_loc_idx = 0;
		while (init_loc_idx < n - MIN_T + 1) {
			if (buffer[init_loc_idx + loc_offset].location - buffer[init_loc_idx].location < LOC_R) {
				if (buffer[init_loc_idx + loc_offset].strand == buffer[init_loc_idx].strand) {
					loc_counter++;
					if (loc_counter == MIN_T) {
						loc_buffer[locs->n] = buffer[init_loc_idx].location;
						locs->n++;
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

		// Store the locations
		locs->a = (uint32_t *)malloc(locs->n * sizeof(uint32_t));
		for (size_t i = 0; i < locs->n; i++) {
			locs->a[i] = loc_buffer[i];
		}
	}
}
