#include "seeding.h"
#include "extraction.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define LOCATION_BUFFER_SIZE 200000

void seeding(index_t idx, char *read, location_v *locs) {
	min_stra_v p; // Buffer which stores the minimizers and their strand
	p.n = 0;
	extract_minimizers(read, &p);

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
