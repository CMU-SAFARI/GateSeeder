#ifndef FORMATING_H
#define FORMATING_H

#include "demeter_util.h"
#include <stdint.h>

typedef struct {
	uint32_t q_start;
	uint32_t q_end;
	int str : 1;
	uint64_t t_start;
	uint64_t t_end;
	uint32_t vt_score;
} record_t;

typedef struct {
	read_metadata_t metadata;
	record_t *record;
	unsigned nb_records;
} record_v;

void paf_write(const record_v r, const uint32_t batch_id);
void paf_batch_done(const uint32_t batch_id);
void paf_write_destroy();

#endif
