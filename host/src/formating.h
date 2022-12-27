#ifndef FORMATING_H
#define FORMATING_H

#include <stdint.h>

typedef struct {
	uint32_t qstart;
	uint32_t qend;
	unsigned str : 1;
	uint32_t t_id;
	uint32_t tstart;
	uint32_t tend;
} record_t;

typedef struct {
	uint32_t read_id;
	const char *qname;
	uint32_t qlen;
	record_t *records;
	unsigned nb_records;
} record_v;

void paf_write_init();
void paf_write(const record_v out);
void paf_write_destroy();

#endif
