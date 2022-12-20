#ifndef PROFILE_H
#define PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif
void print_profile();

#ifdef PROFILE
#include <stdatomic.h>
#include <time.h>

extern atomic_uint_least64_t pf_host_dev;
extern atomic_uint_least64_t pf_krnl;
extern atomic_uint_least64_t pf_dev_host;
extern atomic_uint_least64_t pf_read;

#define PROF_INIT struct timespec start, end
#define PROF_START clock_gettime(CLOCK_MONOTONIC, &start)
#define PROF_END(prof)                                                                                                 \
	{                                                                                                              \
		clock_gettime(CLOCK_MONOTONIC, &end);                                                                  \
		atomic_fetch_add(&prof, (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec));     \
	}

#else
#define PROF_INIT
#define PROF_START
#define PROF_END(prof)
#endif

#ifdef __cplusplus
}
#endif
#endif
