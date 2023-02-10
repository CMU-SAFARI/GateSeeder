#ifndef CPU_H
#define CPU_H
#include "demeter_util.h"

void cpu_pipeline(const read_buf_t input, const unsigned nb_threads, const index_t index);

#endif
