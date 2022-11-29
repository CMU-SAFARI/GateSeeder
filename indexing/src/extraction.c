#include "extraction.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inline void push_seed(key_v *const minimizers, const seed_t minimizer, const unsigned chrom_id, const unsigned b) {

	const uint32_t bucket_mask = (1 << b) - 1;
	if (minimizers->capacity == minimizers->len) {
		minimizers->capacity *= 2;
		REALLOC(minimizers->keys, keym_t, minimizers->capacity);
	}
	minimizers->keys[minimizers->len] = (keym_t){.loc       = minimizer.loc,
	                                             .bucket_id = minimizer.hash & bucket_mask,
	                                             .seed_id   = minimizer.hash >> b,
	                                             .chrom_id  = chrom_id,
	                                             .str       = minimizer.str};
	minimizers->len++;

	/*
	keym_t key = minimizers->keys[minimizers->len - 1];
	printf("loc: %x, hash: %lx, bucket_id: %x, seed_id: %x, chrom_id: %x, str: %x\n", key.loc, minimizer.hash,
	       key.bucket_id, key.seed_id, key.chrom_id, key.str);
	       */
}

static inline uint64_t hash64(uint64_t key, const uint64_t mask) {
	key = (~key + (key << 21)) & mask; // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)) & mask; // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)) & mask; // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31)) & mask;
	return key;
}

void extract_seeds(const uint8_t *seq, const uint32_t len, const unsigned chrom_id, key_v *const minimizers,
                   const unsigned w, const unsigned k, const unsigned b) {
	const uint64_t mask  = (1ULL << 2 * k) - 1;
	const unsigned shift = 2 * (k - 1);
	uint64_t kmer[2]     = {0, 0};
	seed_t buf[256];
	unsigned int l = 0; // l counts the number of bases and is reset to 0 each
	                    // time there is an ambiguous base

	unsigned buf_pos = 0;
	unsigned min_pos = 0;
	seed_t minimizer = {.hash = UINT64_MAX, .loc = 0, .str = 0};

	for (uint32_t i = 0; i < len; i++) {
		uint8_t c           = seq[i];
		seed_t current_seed = {.hash = UINT64_MAX, .loc = 0, .str = 0};
		if (c < 4) {                                            // not an ambiguous base
			kmer[0] = (kmer[0] << 2 | c) & mask;            // forward k-mer
			kmer[1] = (kmer[1] >> 2) | (2ULL ^ c) << shift; // reverse k-mer
			l++;
			if (l >= k) {
				// skip "symmetric k-mers"
				if (kmer[0] == kmer[1]) {
					current_seed.hash = UINT64_MAX;
				} else {
					unsigned char z   = kmer[0] > kmer[1]; // strand
					current_seed.hash = hash64(kmer[z], mask);
					current_seed.loc  = i;
					current_seed.str  = z;
				}
			}
		} else {
			if (l >= w + k - 1 && minimizer.hash != UINT64_MAX) {
				push_seed(minimizers, minimizer, chrom_id, b);
			}
			l = 0;
		}
		buf[buf_pos] = current_seed;

		if (current_seed.hash <= minimizer.hash) {
			if (l >= w + k && minimizer.hash != UINT64_MAX) {
				push_seed(minimizers, minimizer, chrom_id, b);
			}
			minimizer = current_seed;
			min_pos   = buf_pos;

		} else if (buf_pos == min_pos) {
			if (l >= w + k - 1) {
				push_seed(minimizers, minimizer, chrom_id, b);
			}
			minimizer.hash = UINT64_MAX;
			for (unsigned j = buf_pos + 1; j < w; j++) {
				if (minimizer.hash >= buf[j].hash) {
					minimizer = buf[j];
					min_pos   = j;
				}
			}
			for (unsigned j = 0; j <= buf_pos; j++) {
				if (minimizer.hash >= buf[j].hash) {
					minimizer = buf[j];
					min_pos   = j;
				}
			}
			if (l >= w + k - 1 && minimizer.hash != UINT64_MAX) {
				for (unsigned j = buf_pos + 1; j < w; ++j) {
					if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
						push_seed(minimizers, buf[j], chrom_id, b);
					}
				}
				for (unsigned j = 0; j < buf_pos; ++j) {
					if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
						push_seed(minimizers, buf[j], chrom_id, b);
					}
				}
			}
		}

		// If it's the first window
		if (l == w + k - 1 && minimizer.hash != UINT64_MAX) {
			for (unsigned j = buf_pos + 1; j < w; ++j) {
				if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
					push_seed(minimizers, buf[j], chrom_id, b);
				}
			}
			for (unsigned j = 0; j < buf_pos; ++j) {
				if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
					push_seed(minimizers, buf[j], chrom_id, b);
				}
			}
		}
		buf_pos = (buf_pos == w - 1) ? 0 : buf_pos + 1;
	}
	if (l >= w + k - 1 && minimizer.hash != UINT64_MAX) {
		push_seed(minimizers, minimizer, chrom_id, b);
	}
}
