#ifndef FORMATING_H
#define FORMATING_H

#include "GateSeeder_util.h"
#include <stdint.h>

typedef struct {
	uint64_t t_start;
	uint64_t t_end;
	uint32_t q_start;
	uint32_t q_end;
	uint32_t vt_score;
	int8_t str;
} record_t;

typedef struct {
	read_metadata_t metadata;
	record_t *record;
	uint32_t nb_records;
} record_v;

typedef struct {
	uint32_t nb_seq;
	char **seq_name;
	uint32_t *seq_len;
} target_t;

void paf_write(const record_v r, const uint32_t batch_id);
void paf_batch_set_full(const uint32_t batch_id);
void paf_write_destroy();

#endif
