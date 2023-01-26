#include "formating.h"
#include <math.h>

extern uint32_t SE_K;

extern FILE *OUTPUT;
extern target_t TARGET;

typedef struct {
	uint32_t batch_id;
	record_v *read;
	uint32_t len;
	uint32_t capacity;
	unsigned is_full : 1;
} record_buf_t;

// buf is sorted depending on the batch_id
static record_buf_t *paf_buf     = NULL;
static uint32_t paf_buf_capacity = 0;
static uint32_t paf_buf_len      = 0;
static uint32_t cur_batch_id     = 0;

#define TARGET_ID(x) (x >> 30)
#define TARGET_POS(x) (x & ((1 << 30) - 1))
static void paf_print(const record_v r) {
	// Set the mapping quality
	unsigned mapq = 0;
	if (r.nb_records > 1) {
		const unsigned vt_score_0  = r.record[0].vt_score;
		const unsigned vt_score_1  = r.record[1].vt_score;
		const double normalization = (double)(vt_score_0 < 40) ? vt_score_0 : 40;
		mapq = (unsigned)((1.0 - (double)vt_score_1 / (double)vt_score_0) * log((double)vt_score_0) *
		                  normalization);
		if (mapq > 60) {
			mapq = 60;
		}
	} else if (r.nb_records == 1) {
		const unsigned vt_score_0  = r.record[0].vt_score;
		const double normalization = (double)(vt_score_0 < 40) ? vt_score_0 : 40;
		mapq                       = (unsigned)(log((double)vt_score_0) * normalization);
		if (mapq > 60) {
			mapq = 60;
		}
	}

	for (uint32_t i = 0; i < r.nb_records; i++) {
		const record_t record     = r.record[i];
		char *const target_name   = TARGET.seq_name[TARGET_ID(record.t_start)];
		const uint32_t target_len = TARGET.seq_len[TARGET_ID(record.t_start)];
		const uint32_t t_start    = TARGET_POS(record.t_start) - (SE_K - 1);
		const uint32_t t_end      = TARGET_POS(record.t_end);
		fprintf(OUTPUT, "%s\t%u\t%u\t%u\t%c\t%s\t%u\t%u\t%u\t%u\t%u\t%u\tvt:i:%u\n", r.metadata.name,
		        r.metadata.len, record.q_start - (SE_K - 1), record.q_end, "+-"[record.str], target_name,
		        target_len, t_start, t_end, record.vt_score * SE_K, t_end - t_start, (i == 0) ? mapq : 0,
		        record.vt_score);
	}
	free(r.record);
	free(r.metadata.name);
}

static void paf_store(const record_v r, const uint32_t batch_id) {
	// Check if there is already a buffer for this batch
	for (uint32_t i = 0; i < paf_buf_len; i++) {
		if (paf_buf[i].batch_id == batch_id) {
			// Push the read
			if (paf_buf[i].len == paf_buf[i].capacity) {
				paf_buf[i].capacity = paf_buf[i].capacity << 1;
				REALLOC(paf_buf[i].read, record_v, paf_buf[i].capacity);
			}
			paf_buf[i].read[paf_buf[i].len] = r;
			paf_buf[i].len++;
			return;
		}
	}
	// Create a new buffer for the batch

	const uint32_t buf_capacity = paf_buf_capacity ? paf_buf[0].capacity : 1;

	if (paf_buf_len == paf_buf_capacity) {
		paf_buf_capacity++;
		REALLOC(paf_buf, record_buf_t, paf_buf_capacity);
	}

	paf_buf[paf_buf_len] = (record_buf_t){.batch_id = batch_id, .len = 1, .capacity = buf_capacity, .is_full = 0};
	MALLOC(paf_buf[paf_buf_len].read, record_v, paf_buf[paf_buf_len].capacity);
	paf_buf[paf_buf_len].read[0] = r;

	// Sort the buffers according to the batch_id
	for (uint32_t i = paf_buf_len; i > 0; i--) {
		if (paf_buf[i].batch_id < paf_buf[i - 1].batch_id) {
			const record_buf_t tmp = paf_buf[i];
			paf_buf[i]             = paf_buf[i - 1];
			paf_buf[i - 1]         = tmp;
		} else {
			break;
		}
	}
	paf_buf_len++;
}

void paf_write(const record_v r, const uint32_t batch_id) {
	flockfile(OUTPUT);
	if (cur_batch_id == batch_id) {
		// Print the current record
		paf_print(r);
	} else {
		// Store the current record
		paf_store(r, batch_id);
	}
	funlockfile(OUTPUT);
}

static void write_buf() {
	uint32_t i = 0;
	for (uint32_t j = 0; j < paf_buf_len; j++) {
	}
	for (; i < paf_buf_len; i++) {
		if (paf_buf[i].batch_id == cur_batch_id) {
			for (uint32_t j = 0; j < paf_buf[i].len; j++) {
				paf_print(paf_buf[i].read[j]);
			}
			if (paf_buf[i].is_full) {
				cur_batch_id++;
			} else {
				i++;
				break;
			}
		} else {
			break;
		}
	}
	for (uint32_t j = i; j < paf_buf_len; j++) {
		record_buf_t tmp = paf_buf[j - i];
		paf_buf[j - i]   = paf_buf[j];
		paf_buf[j]       = tmp;
	}
	paf_buf_len -= i;
}

void paf_batch_set_full(const uint32_t batch_id) {
	flockfile(OUTPUT);
	if (cur_batch_id == batch_id) {
		cur_batch_id++;
		write_buf();
	} else {
		for (uint32_t i = 0; i < paf_buf_len; i++) {
			if (paf_buf[i].batch_id == batch_id) {
				paf_buf[i].is_full = 1;
				break;
			}
		}
	}
	funlockfile(OUTPUT);
}

void paf_write_destroy() {
	for (uint32_t i = 0; i < paf_buf_capacity; i++) {
		free(paf_buf[i].read);
	}
	free(paf_buf);
}
