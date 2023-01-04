#include "formating.h"

extern FILE *OUTPUT;

typedef struct {
	uint32_t batch_id;
	record_v *read;
	uint32_t len;
	uint32_t capacity;
} record_buf_t;

static record_buf_t *paf_buf;
static uint32_t paf_buf_capacity;

void paf_write_init() {
	paf_buf_capacity = 0;
	paf_buf          = NULL;
}

void paf_print(const record_v r) {}

// TODO: function to increment the cur_batch_id

void paf_write(const record_v r, const uint32_t batch_id) {
	static uint32_t cur_batch_id = 0;
	flockfile(OUTPUT);
	if (cur_batch_id == batch_id) {
		// First check if this batch is bufferized
		for (uint32_t i = 0; i < paf_buf_capacity; i++) {
			if (paf_buf[i].batch_id == batch_id && paf_buf[i].len != 0) {
				for (uint32_t j = 0; j < paf_buf[i].len; j++) {
					paf_print(paf_buf[i].read[j]);
				}
				paf_buf[i].len = 0;
				break;
			}
		}
		// Second print the current records
		paf_print(r);
	}
	funlockfile(OUTPUT);
}

void paf_write_destroy() { free(paf_buf); }
