#include "extraction.h"

unsigned char seq_nt4_table[256] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

static inline uint64_t hash64(uint64_t key, uint64_t mask) {
	key = (~key + (key << 21)) & mask; // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)) & mask; // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)) & mask; // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31)) & mask;
	return key;
}

static inline void push_min_loc_stra(min_loc_stra_v *p, const kmer_loc_stra_t min, uint32_t mask, uint32_t offset) {
	p->a[p->n] = (min_loc_stra_t){min.kmer & mask, min.loc + offset, min.stra};
	p->n++;
}

void extract_minimizers(const char *dna, unsigned int len, unsigned int w, unsigned int k, const unsigned int b,
                        min_loc_stra_v *p, uint32_t offset) {
	uint64_t shift1          = 2 * (k - 1);
	uint64_t mask            = (1ULL << 2 * k) - 1;
	uint64_t kmer[2]         = {0, 0};
	uint32_t mask1           = (1ULL << b) - 1;
	uint32_t last_loc        = UINT32_MAX;
	size_t l                 = 0;
	size_t buf_pos           = 0;
	size_t min_pos           = 0;
	kmer_loc_stra_t buf[256] = {(kmer_loc_stra_t){UINT64_MAX, UINT32_MAX, 1}};
	kmer_loc_stra_t min      = {UINT64_MAX, UINT32_MAX, 1};

	for (size_t i = 0; i < len; ++i) {
		uint64_t c           = (uint64_t)seq_nt4_table[(uint8_t)dna[i]];
		kmer_loc_stra_t info = {UINT64_MAX, UINT32_MAX, 1};
		if (c < 4) { // not an ambiguous base
			int z;
			kmer[0] = (kmer[0] << 2 | c) & mask;             // forward k-mer
			kmer[1] = (kmer[1] >> 2) | (3ULL ^ c) << shift1; // reverse k-mer
			if (kmer[0] == kmer[1])
				continue;              // skip "symmetric k-mers" as we don't
				                       // know it strand
			z = kmer[0] < kmer[1] ? 0 : 1; // strand
			++l;
			if (l >= k) {
				info.kmer = hash64(kmer[z], mask);
				info.loc  = (uint32_t)i;
				info.stra = z;
			}
		} else {
			if (l >= w + k - 1) {
				push_min_loc_stra(p, min, mask1, offset);
			}
			l = 0;
		}
		buf[buf_pos] = info;  // need to do this here as appropriate buf_pos
		                      // and buf[buf_pos] are needed below
		                      //
		if (l == w + k - 1) { // special case for the first window
			// - because identical k-mers are not
			// stored yet
			for (size_t j = buf_pos + 1; j < w; ++j)
				if (min.kmer == buf[j].kmer && buf[j].loc != min.loc) {
					push_min_loc_stra(p, buf[j], mask1, offset);
				}
			for (size_t j = 0; j < buf_pos; ++j)
				if (min.kmer == buf[j].kmer && buf[j].loc != min.loc) {
					push_min_loc_stra(p, buf[j], mask1, offset);
				}
		}
		if (info.kmer <= min.kmer) { // a new minimum; then write the old min
			if (l >= w + k) {
				push_min_loc_stra(p, min, mask1, offset);
			}
			min     = info;
			min_pos = buf_pos;
		} else if (buf_pos == min_pos) { // old min has moved outside the window
			if (l >= w + k - 1) {
				push_min_loc_stra(p, min, mask1, offset);
			}
			min.kmer = UINT64_MAX;
			for (size_t j = buf_pos + 1; j < w; ++j) {
				if (min.kmer >= buf[j].kmer) {
					min     = buf[j];
					min_pos = j;
				}
			}
			for (size_t j = 0; j <= buf_pos; ++j) {
				if (min.kmer >= buf[j].kmer) {
					min     = buf[j];
					min_pos = j;
				}
			}

			if (l >= w + k - 1) {
				for (size_t j = buf_pos + 1; j < w; ++j) {
					if (min.kmer == buf[j].kmer && min.loc != buf[j].loc) {
						push_min_loc_stra(p, buf[j], mask1, offset);
					}
				}
				for (size_t j = 0; j <= buf_pos; ++j) {
					if (min.kmer == buf[j].kmer && min.loc != buf[j].loc) {
						push_min_loc_stra(p, buf[j], mask1, offset);
					}
				}
			}
		}
		buf_pos = (buf_pos == w - 1) ? 0 : buf_pos + 1;
	}
	if (l >= w + k - 1) {
		push_min_loc_stra(p, min, mask1, offset);
	}
}
