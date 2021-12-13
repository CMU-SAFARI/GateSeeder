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
	uint32_t *location_buffer[2];
	location_buffer[0] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	location_buffer[1] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	location_v locs;
	locs.a = (buffer_t *)malloc(LOCATION_BUFFER_SIZE * sizeof(buffer_t));
	for (size_t i = param->start; i < param->end; i++) {
#ifdef VARIABLE_LEN
		seeding(param->idx, param->reads.a[i], &locs, param->reads.len[i], location_buffer);
#else
		seeding(param->idx, param->reads.a[i], &locs, location_buffer);
#endif
		for (size_t j = 0; j < locs.n; j++) {
			char strand = locs.a[j].strand ? '-' : '+';
			if (j == 0) {
				fprintf(param->fp, "%c.%u", strand, locs.a[0].location);
			} else {
				fprintf(param->fp, "\t%c.%u", strand, locs.a[j].location);
			}
		}
		fputs("\n", param->fp);
	}
	return (void *)NULL;
}

#else

void read_seeding(const index_t idx, const read_v reads, FILE *fp) {
	uint32_t *location_buffer[2];
	location_buffer[0] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	location_buffer[1] = (uint32_t *)malloc(sizeof(uint32_t) * LOCATION_BUFFER_SIZE);
	location_v locs;
	locs.a = (buffer_t *)malloc(LOCATION_BUFFER_SIZE * sizeof(buffer_t));
	for (size_t i = 0; i < reads.n; i++) {
#ifdef VARIABLE_LEN
		seeding(idx, reads.a[i], &locs, reads.len[i], location_buffer);
#else
		seeding(idx, reads.a[i], &locs, location_buffer);
#endif
		for (size_t j = 0; j < locs.n; j++) {
			char strand = locs.a[j].strand ? '-' : '+';
			if (j == 0) {
				fprintf(fp, "%c.%u", strand, locs.a[0].location);
			} else {
				fprintf(fp, "\t%c.%u", strand, locs.a[j].location);
			}
		}
		fputs("\n", fp);
	}
}

#endif

#ifdef VARIABLE_LEN
void seeding(const index_t idx, const char *read, location_v *locs, size_t len, uint32_t *location_buffer[2]) {
	min_stra_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;
	extract_minimizers(read, &p, len);
#else
void seeding(const index_t idx, const char *read, location_v *locs, uint32_t *location_buffer[2]) {
	min_stra_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;
	extract_minimizers(read, &p);
#endif
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

	size_t n = location_buffer_len[sel];

	locs->n = n;
	for (size_t i = 0; i < n; i++) {
			locs->a[i].location = location_buffer[sel][i] & (UINT32_MAX - 1);
			locs->a[i].strand   = location_buffer[sel][i] & 1;
	}
	/*
	// Adjacency test
	locs->n = 0;
	if (n >= 3) {
		// TODO: maybe can be optimized
		buffer_t buffer[LOCATION_BUFFER_SIZE];
		for (size_t i = 0; i < n; i++) {
			buffer[i].location = location_buffer[sel][i] & (UINT32_MAX - 1);
			buffer[i].strand   = location_buffer[sel][i] & 1;
		}
		size_t loc_counter  = 1;
		size_t loc_offset   = 1;
		size_t init_loc_idx = 0;
		while (init_loc_idx < n - MIN_T + 1) {
			if (buffer[init_loc_idx + loc_offset].location - buffer[init_loc_idx].location < LOC_R) {
				if (buffer[init_loc_idx + loc_offset].strand == buffer[init_loc_idx].strand) {
					loc_counter++;
					if (loc_counter == MIN_T) {
						locs->a[locs->n] = buffer[init_loc_idx];
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
	}
	*/
}
