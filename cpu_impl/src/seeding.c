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
	uint32_t mem_buffer[2][2000]; // Buffers used to store the locations(and the
	                              // corresponding strand) returned by the index
	uint16_t mem_buffer_len[2];

	for (size_t i = 0; i <= p.n; i++) {
		unsigned char sel = i % 2;
		// Query the locations and the strands from the index and store them
		// into one of mem_buffer
		if (i != p.n) {
			size_t mem_buffer_i = 0;
			uint32_t minimizer  = p.a[i].minimizer;
			uint32_t min        = (minimizer == 0) ? 0 : idx.h[minimizer - 1];
			uint32_t max        = idx.h[minimizer];
			mem_buffer_len[sel] = max - min;
			for (uint32_t j = min; j < max; j++) {
				mem_buffer[sel][mem_buffer_i] = idx.location[j] ^ p.a[i].strand;
				mem_buffer_i++;
			}
		}
		// Merge the previously loaded mem_buffer with one of the
		// location_buffer in the other location_buffer
		if (i != 0) {
			size_t loc_i = 0;
			size_t mem_i = 0;
			size_t len   = 0;
			while (loc_i < location_buffer_len[sel] && mem_i < mem_buffer_len[1 - sel]) {
				if (location_buffer[sel][loc_i] <= mem_buffer[1 - sel][mem_i]) {
					location_buffer[1 - sel][len] = location_buffer[sel][loc_i];
					len++;
					loc_i++;
				} else {
					location_buffer[1 - sel][len] = mem_buffer[1 - sel][mem_i];
					len++;
					mem_i++;
				}
			}
			if (loc_i == location_buffer_len[sel]) {
				while (mem_i != mem_buffer_len[1 - sel]) {
					location_buffer[1 - sel][len] = mem_buffer[1 - sel][mem_i];
					len++;
					mem_i++;
				}
			} else {
				while (loc_i != location_buffer_len[sel]) {
					location_buffer[1 - sel][len] = location_buffer[sel][loc_i];
					len++;
					loc_i++;
				}
			}
			location_buffer_len[1 - sel] = len;
		}
	}

	size_t n = location_buffer_len[1 - p.n % 2];
	// Adjacency test
	locs->n = 0;
	if (n >= 3) {
		buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t) * n);
		for (size_t i = 0; i < n; i++) {
			buffer[i].location = location_buffer[1 - p.n % 2][i] & (UINT32_MAX - 1);
			buffer[i].strand   = location_buffer[1 - p.n % 2][i] & 1;
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
		free(buffer);

		// Store the solution
		locs->a = (uint32_t *)malloc(locs->n * sizeof(uint32_t));
		for (size_t i = 0; i < locs->n; i++) {
			locs->a[i] = loc_buffer[i];
		}
	}
}
