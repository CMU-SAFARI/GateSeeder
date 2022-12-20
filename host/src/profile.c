#include "profile.h"
#include <stdint.h>
#include <stdio.h>

#ifdef PROFILE
atomic_uint_least64_t pf_host_dev = 0;
atomic_uint_least64_t pf_krnl     = 0;
atomic_uint_least64_t pf_dev_host = 0;
atomic_uint_least64_t pf_read     = 0;

void print_profile() {
	fprintf(stderr, "[PROFILING] host_dev time: %lu ns\n", pf_host_dev);
	fprintf(stderr, "[PROFILING] krnl time: %lu ns\n", pf_krnl);
	fprintf(stderr, "[PROFILING] dev_host time: %lu ns\n", pf_dev_host);
	fprintf(stderr, "[PROFILING] read time: %lu ns\n", pf_read);
}

#else
void print_profile() {}
#endif
