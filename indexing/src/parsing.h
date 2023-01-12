#ifndef PARSING_H
#define PARSING_H

#include <stdint.h>

typedef struct {
	unsigned nb_sequences;
	uint32_t *len;
	uint8_t **seq;
	char **name;
} target_t;

target_t parse_target(const char *const file_name);
#endif
