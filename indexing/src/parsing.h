#ifndef PARSING_H
#define PARSING_H

#include "types.h"

target_t parse_target(int fd);
void target_destroy(const target_t target);

#endif
