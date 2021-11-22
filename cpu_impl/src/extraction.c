#include "extraction.h"
#include "seeding.h"
#include <stdio.h>
#include <string.h>
#define SHIFT1 (2 * (K - 1))
#define MASK ((1ULL << 2 * K) - 1)
#define MASK_B ((1ULL << B) - 1)
static unsigned char seq_nt4_table[256] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

static inline uint64_t hash64(uint64_t key) {
	key = (~key + (key << 21)) & MASK; // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)) & MASK; // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)) & MASK; // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31)) & MASK;
	return key;
}

void extract_minimizers(const char *read, min_stra_v *p) {
	uint64_t kmer[2] = {0, 0};
	min_stra_reg_t buff[256];
	unsigned int l = 0; // l counts the number of bases and is reset to 0 each
	                    // time there is an ambiguous base

	size_t buff_pos         = 0;
	unsigned int min_pos    = 0;
	min_stra_reg_t min_reg  = {.minimizer = UINT64_MAX, .strand = 0};
	unsigned char min_saved = 0;
	unsigned char same_min  = 0;

	for (size_t i = 0; i < READ_LEN; ++i) {
		unsigned char c         = seq_nt4_table[(uint8_t)read[i]];
		min_stra_reg_t hash_reg = {.minimizer = UINT64_MAX, .strand = 0};
		if (c < 4) {                                             // not an ambiguous base
			kmer[0] = (kmer[0] << 2 | c) & MASK;             // forward k-mer
			kmer[1] = (kmer[1] >> 2) | (3ULL ^ c) << SHIFT1; // reverse k-mer
			if (kmer[0] == kmer[1]) {
				continue; // skip "symmetric k-mers" as we don't
				          // know it strand
			}
			unsigned char z = kmer[0] > kmer[1]; // strand
			++l;
			if (l >= K) {
				hash_reg.minimizer = hash64(kmer[z]);
				hash_reg.strand    = z;
			}
		} else {
			if (l >= W + K - 1) {
				push_min_stra(p, min_reg.minimizer, min_reg.strand);
				min_saved = 1;
			}
			l = 0;
		}
		buff[buff_pos] = hash_reg;

		if (l == W + K - 1) {
			if (same_min) {
				push_min_stra(p, min_reg.minimizer, !min_reg.strand);
				same_min = 0;
			}
		}

		if (hash_reg.minimizer <= min_reg.minimizer) {
			if (l >= W + K) {
				push_min_stra(p, min_reg.minimizer, min_reg.strand);
			} else if (l < W + K - 1 && hash_reg.minimizer == min_reg.minimizer &&
			           hash_reg.strand != min_reg.strand) {
				same_min = 1;
			} else {
				same_min = 0;
			}
			min_reg   = hash_reg;
			min_pos   = buff_pos;
			min_saved = 0;
		} else if (buff_pos == min_pos) {
			if (l >= W + K - 1) {
				push_min_stra(p, min_reg.minimizer, min_reg.strand);
			}
			min_reg.minimizer = UINT64_MAX;
			char same_min_w   = 0;
			for (size_t j = buff_pos + 1; j < W; j++) {
				if (min_reg.minimizer > buff[j].minimizer) {
					min_reg    = buff[j];
					min_pos    = j;
					min_saved  = 0;
					same_min_w = 0;
				} else if (min_reg.minimizer == buff[j].minimizer && min_reg.minimizer != UINT64_MAX) {
					min_pos = j;
					if (min_reg.strand != buff[j].strand) {
						same_min_w = 1;
					}
				}
			}
			for (size_t j = 0; j <= buff_pos; j++) {
				if (min_reg.minimizer > buff[j].minimizer) {
					min_pos    = j;
					min_reg    = buff[j];
					same_min_w = 0;
					min_saved  = 0;
				} else if (min_reg.minimizer == buff[j].minimizer && min_reg.minimizer != UINT64_MAX) {
					min_pos = j;
					if (min_reg.strand != buff[j].strand) {
						same_min_w = 1;
					}
				}
			}
			if (same_min_w && l >= W + K - 1) {
				push_min_stra(p, min_reg.minimizer, !min_reg.strand);
			}
		}
		buff_pos = (buff_pos == W - 1) ? 0 : buff_pos + 1;
	}
	if (min_reg.minimizer != UINT64_MAX && !min_saved) {
		push_min_stra(p, min_reg.minimizer, min_reg.strand);
	}
}

inline void push_min_stra(min_stra_v *p, uint64_t min, uint8_t stra) {
	uint32_t min_b = min & MASK_B;
	for (size_t i = 0; i < p->n; i++) {
		if (p->a[i].minimizer == min_b && p->a[i].strand == stra) {
			return;
		}
	}
	p->a[p->n].minimizer = min_b;
	p->a[p->n].strand    = stra;
	p->n++;
}
