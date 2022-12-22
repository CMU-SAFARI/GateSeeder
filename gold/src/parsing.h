#ifndef PARSING_DEBUG_H
#define PARSING_DEBUG_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
	uint32_t id;
	uint32_t len;
	char *name;
	uint8_t *seq;
	char *qual;
} read_t;

typedef struct {
	unsigned nb_reads;
	read_t *reads;
} read_v;

void open_reads(int fd);
void parse_reads(read_v *input);

#endif
