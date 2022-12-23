#include "demeter_util.h"
#include "extraction.h"

extern unsigned SE_K;
extern unsigned SE_W;

static unsigned SHIFT;
static uint64_t MASK;

void extraction_init() {
	SHIFT = 2 * (SE_K - 1);
	MASK  = (1ULL << 2 * SE_K) - 1;
}

int push_seed_mapping(seed_v *const minimizers, const seed_t minimizer, const int partial) {
	if (partial) {
		minimizers->seeds[minimizers->len] = minimizer;
		minimizers->len++;
		return (minimizers->len == minimizers->capacity);
	}
	if (minimizers->capacity == minimizers->len) {
		minimizers->capacity = (minimizers->capacity == 0) ? 1 : minimizers->capacity << 1;
		REALLOC(minimizers->seeds, seed_t, minimizers->capacity);
	}
	minimizers->seeds[minimizers->len] = minimizer;
	minimizers->len++;
	return 0;
}

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

uint32_t extract_seeds_mapping(const uint8_t *seq, const uint32_t len, seed_v *const minimizers, int partial) {
	uint64_t kmer[2] = {0, 0};
	seed_t buf[256];
	unsigned int l = 0; // l counts the number of bases and is reset to 0 each
	                    // time there is an ambiguous base

	unsigned buf_pos = 0;
	unsigned min_pos = 0;
	seed_t minimizer = {.hash = UINT64_MAX, .loc = 0, .str = 0};

	minimizers->len = 0;

	for (uint32_t i = 0; i < len; i++) {
		uint8_t c           = seq[i];
		seed_t current_seed = {.hash = UINT64_MAX, .loc = 0, .str = 0};
		if (c < 4) {                                            // not an ambiguous base
			kmer[0] = (kmer[0] << 2 | c) & MASK;            // forward k-mer
			kmer[1] = (kmer[1] >> 2) | (2ULL ^ c) << SHIFT; // reverse k-mer
			l++;
			if (l >= SE_K) {
				// skip "symmetric k-mers"
				if (kmer[0] == kmer[1]) {
					current_seed.hash = UINT64_MAX;
				} else {
					unsigned char z   = kmer[0] > kmer[1]; // strand
					current_seed.hash = hash64(kmer[z]);
					current_seed.loc  = i;
					current_seed.str  = z;
				}
			}
		} else {
			if (l >= SE_W + SE_K - 1 && minimizer.hash != UINT64_MAX) {
				if (push_seed_mapping(minimizers, minimizer, partial)) {
					return minimizer.loc - SE_K + 2;
				}
			}
			l = 0;
		}
		buf[buf_pos] = current_seed;

		if (current_seed.hash <= minimizer.hash) {
			if (l >= SE_W + SE_K && minimizer.hash != UINT64_MAX) {
				if (push_seed_mapping(minimizers, minimizer, partial)) {
					return minimizer.loc - SE_K + 2;
				}
			}
			minimizer = current_seed;
			min_pos   = buf_pos;

		} else if (buf_pos == min_pos) {
			if (l >= SE_W + SE_K - 1) {
				if (push_seed_mapping(minimizers, minimizer, partial)) {
					return minimizer.loc - SE_K + 2;
				}
			}
			minimizer.hash = UINT64_MAX;
			for (unsigned j = buf_pos + 1; j < SE_W; j++) {
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
			if (l >= SE_W + SE_K - 1 && minimizer.hash != UINT64_MAX) {
				for (unsigned j = buf_pos + 1; j < SE_W; ++j) {
					if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
						if (push_seed_mapping(minimizers, buf[j], partial)) {
							return buf[j].loc - SE_K + 2;
						}
					}
				}
				for (unsigned j = 0; j < buf_pos; ++j) {
					if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
						if (push_seed_mapping(minimizers, buf[j], partial)) {
							return buf[j].loc - SE_K + 2;
						}
					}
				}
			}
		}

		// If it's the first window
		if (l == SE_W + SE_K - 1 && minimizer.hash != UINT64_MAX) {
			for (unsigned j = buf_pos + 1; j < SE_W; ++j) {
				if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
					if (push_seed_mapping(minimizers, buf[j], partial)) {
						return buf[j].loc - SE_K + 2;
					}
				}
			}
			for (unsigned j = 0; j < buf_pos; ++j) {
				if (minimizer.hash == buf[j].hash && minimizer.loc != buf[j].loc) {
					if (push_seed_mapping(minimizers, buf[j], partial)) {
						return buf[j].loc - SE_K + 2;
					}
				}
			}
		}
		buf_pos = (buf_pos == SE_W - 1) ? 0 : buf_pos + 1;
	}
	if (l >= SE_W + SE_K - 1 && minimizer.hash != UINT64_MAX) {
		if (push_seed_mapping(minimizers, minimizer, partial)) {
			return minimizer.loc - SE_K + 2;
		}
	}
	return len;
}
