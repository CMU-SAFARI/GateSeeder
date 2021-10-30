#include "compare.h"
#include <stdlib.h>
#include <string.h>

void compare(target_v tar, read_v reads, cindex_t idx, const size_t len,
             const unsigned int w, const unsigned int k, const unsigned int b,
             const unsigned int min_t, const unsigned int loc_r) {
	unsigned int tp_counter         = 0;
	unsigned int um_counter         = 0;
	unsigned int m_counter          = 0;
	unsigned int fn_counter         = 0;
	unsigned int loc_counter        = 0;
	unsigned int quality_counter_tn = 0;
	unsigned int quality_counter_tp = 0;
	location_v locs;
	char flag        = 0;
	char buff[50000] = {0};
	size_t j         = 0;
	for (size_t i = 0; i < reads.n; i++) {
		cseeding(idx, reads.a[i], len, w, k, b, min_t, loc_r, &locs);
		if (j < tar.n) {
			if (strcmp(tar.a[j].name, reads.name[i]) == 0) {
				loc_counter += tar.a[j].n;
				for (size_t k = 0; k < locs.n; k++) {
					flag = 0;
					for (size_t l = 0; l < tar.a[j].n; l++) {
						if (locs.a[k] >= (tar.a[j].a[l].start) &&
						    locs.a[k] <= (tar.a[j].a[l].end)) {
							tp_counter++;
							quality_counter_tp += tar.a[j].a[l].quality;
							flag    = 1;
							buff[l] = 1;
						}
					}
					if (!flag) {
						um_counter++;
					} else {
						m_counter++;
					}
				}
				for (size_t l = 0; l < tar.a[j].n; l++) {
					if (buff[l]) {
						buff[l] = 0;
					} else {
						fn_counter++;
						quality_counter_tn += tar.a[j].a[l].quality;
					}
				}
				j++;
			} else {
				um_counter += locs.n;
			}
		} else {
			um_counter += locs.n;
		}
	}
	printf("Info: Number of true positives %u (%f%%)\n", tp_counter,
	       ((float)loc_counter - fn_counter) / loc_counter * 100);
	printf("Info: Average mapping quality of the true positives %u\n",
	       quality_counter_tp / tp_counter);
	printf("Info: Number of false negatives %u (%f%%)\n", fn_counter,
	       ((float)fn_counter) / loc_counter * 100);
	if (fn_counter) {
		printf("Info: Average mapping quality of the false negatives %u\n",
		       quality_counter_tn / fn_counter);
	}
	printf("Info: Number of unmatching locations %u\n", um_counter);
	printf("Info: Number of found locations %u\n", um_counter + m_counter);
	printf("Info: Percentage of matching locations compared to the found "
	       "locations "
	       "%f%%\n",
	       ((float)m_counter) / (um_counter + m_counter) * 100);
}
