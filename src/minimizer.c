#include "minimizer.h"
#include <string.h>

static unsigned char seq_nt4_table[256] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

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

void get_minimizers(const char *read, const size_t len, const unsigned int w,
                    const unsigned int k, const unsigned int b, min_stra_v *p) {

    uint64_t shift1 = 2 * (k - 1), mask = (1ULL << 2 * k) - 1, kmer[2] = {0, 0},
             mask1 = (1ULL << b) - 1; // should be defined as const parameters.

    min_stra_t buff[256]; // size:  w, size element: 2 * k
    memset(buff, 0xff, w * 4);

    unsigned int l = 0;  // l counts the number of bases and is reset to 0 each
                         // time there is an ambiguous base, optimal size:
                         // log2(k) + 1 or max = size of the read
    size_t buff_pos = 0; // size: log2(size(buff)) + 1
    unsigned int min_pos = 0;
    min_stra_t min_reg = {.minimizer = UINT32_MAX,
                          .strand = 0}; // size: 2 * k bits
    unsigned char last_min = 0;

    for (unsigned int i = 0; i < len; ++i) {
        unsigned int c = seq_nt4_table[(uint8_t)read[i]]; // should be removed
        if (c < 4) {                             // not an ambiguous base
            int z;                               // size: 2 * k
            kmer[0] = (kmer[0] << 2 | c) & mask; // forward k-mer
            kmer[1] = (kmer[1] >> 2) | (3ULL ^ c) << shift1; // reverse k-mer
            if (kmer[0] == kmer[1])
                continue; // skip "symmetric k-mers" as we don't
                          // know it strand
            z = kmer[0] < kmer[1] ? 0 : 1; // strand
            ++l;
            if (l >= k) {
                min_stra_t hash_reg = {
                    .minimizer = hash64(kmer[z], mask) & mask1, .strand = z};
                buff[buff_pos] = hash_reg;
                if (hash_reg.minimizer <
                    min_reg.minimizer) { // If there is a new minimum
                    if (l >= w + k - 1) {
                        p->a[p->n] = min_reg;
                        p->n++;
                    }
                    last_min = 0;
                    min_pos = buff_pos;
                    min_reg = hash_reg;
                } else if (min_pos == buff_pos) {
                    if (l >= w + k - 1) {
                        p->a[p->n] = min_reg;
                        p->n++;
                    }

                    min_reg.minimizer = UINT32_MAX;
                    for (size_t j = 0; j < w; j++) {
                        if (j != buff_pos &&
                            buff[j].minimizer < min_reg.minimizer) {
                            min_reg = buff[j];
                            min_pos = j;
                        }
                    }
                    last_min = 0;
                }
                buff_pos = (buff_pos == w - 1) ? 0 : buff_pos + 1;
            }
        } else {
            if (l >= w + k - 1) {
                last_min = 1;
                p->a[p->n] = min_reg;
                p->n++;
            }
            l = 0;
            min_pos = 0;
            buff_pos = 0;
        }
    }
    if (min_reg.minimizer != UINT32_MAX && !last_min) {
        p->a[p->n] = min_reg;
        p->n++;
    }
}
