#include "extraction.h"
#include <stdio.h>
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

void extract_minimizers(const char *read, const size_t len,
                        const unsigned int w, const unsigned int k,
                        const unsigned int b, min_stra_v *p) {

    uint64_t shift1 = 2 * (k - 1), mask = (1ULL << 2 * k) - 1, kmer[2] = {0, 0},
             mask1 = (1ULL << b) - 1; // should be defined as const parameters.

    min_stra_reg_t buff[256]; // size:  w

    unsigned int l = 0; // l counts the number of bases and is reset to 0 each
                        // time there is an ambiguous base

    size_t buff_pos = 0; // size: log2(size(buff)) + 1
    unsigned int min_pos = 0;
    min_stra_reg_t min_reg = {.minimizer = UINT64_MAX, .strand = 0};
    unsigned char min_saved = 0;
    unsigned char same_min_count = 0; // size log2(size(buff)) + 1
    unsigned char strand_buff[256];   // size: w, element 1bit
    size_t strand_pos = 0;            // size: log2(size(buff)) + 1

    for (unsigned int i = 0; i < len; ++i) {
        unsigned int c = seq_nt4_table[(uint8_t)read[i]]; // should be removed
        min_stra_reg_t hash_reg = {.minimizer = UINT64_MAX, .strand = 0};
        if (c < 4) {                             // not an ambiguous base
            kmer[0] = (kmer[0] << 2 | c) & mask; // forward k-mer
            kmer[1] = (kmer[1] >> 2) | (3ULL ^ c) << shift1; // reverse k-mer
            if (kmer[0] == kmer[1]) {
                continue; // skip "symmetric k-mers" as we don't
                // know it strand
            }
            unsigned char z = kmer[0] > kmer[1]; // strand
            ++l;
            if (l >= k) {
                hash_reg.minimizer = hash64(kmer[z], mask);
                hash_reg.strand = z;
            }
        } else {
            if (l >= w + k - 1) {
                push_min_stra(p, min_reg.minimizer & mask1, min_reg.strand);
                min_saved = 1;
            }
            l = 0;
        }
        buff[buff_pos] = hash_reg;

        if (l == w + k - 1) { // a new minimum; then write the old min
            while (same_min_count) {
                strand_pos = (strand_pos == 0) ? w - 1 : strand_pos - 1;
                push_min_stra(p, min_reg.minimizer & mask1,
                              strand_buff[strand_pos]);
                same_min_count--;
            }
        }

        if (hash_reg.minimizer <= min_reg.minimizer) {
            if (l >= w + k) {
                push_min_stra(p, min_reg.minimizer & mask1, min_reg.strand);
            } else if (l < w + k - 1 &&
                       hash_reg.minimizer == min_reg.minimizer) {
                strand_buff[strand_pos] = min_reg.strand;
                strand_pos = (strand_pos == w - 1) ? 0 : strand_pos + 1;
                same_min_count++;
            } else {
                same_min_count = 0;
            }
            min_reg = hash_reg;
            min_pos = buff_pos;
            min_saved = 0;
        } else if (buff_pos == min_pos) {
            if (l >= w + k - 1) {
                push_min_stra(p, min_reg.minimizer & mask1, min_reg.strand);
            }
            min_reg.minimizer = UINT64_MAX;
            unsigned char same_min_count_w = 0;
            for (size_t j = buff_pos + 1; j < w; j++) {
                if (min_reg.minimizer > buff[j].minimizer) {
                    min_reg = buff[j];
                    min_pos = j;
                    min_saved = 0;
                    same_min_count_w = 0;
                } else if (min_reg.minimizer == buff[j].minimizer &&
                           min_reg.minimizer != UINT64_MAX) {
                    strand_buff[strand_pos] = min_reg.strand;
                    strand_pos = (strand_pos == w - 1) ? 0 : strand_pos + 1;
                    same_min_count_w++;
                    min_pos = j;
                    min_reg.strand = buff[j].strand;
                }
            }
            for (size_t j = 0; j <= buff_pos; j++) {
                if (min_reg.minimizer > buff[j].minimizer) {
                    min_reg = buff[j];
                    min_pos = j;
                    same_min_count_w = 0;
                    min_saved = 0;
                } else if (min_reg.minimizer == buff[j].minimizer &&
                           min_reg.minimizer != UINT64_MAX) {
                    strand_buff[strand_pos] = min_reg.strand;
                    strand_pos = (strand_pos == w - 1) ? 0 : strand_pos + 1;
                    same_min_count_w++;
                    min_pos = j;
                    min_reg.strand = buff[j].strand;
                }
            }
            while (same_min_count_w && l >= w + k - 1) {
                strand_pos = (strand_pos == 0) ? w - 1 : strand_pos - 1;
                push_min_stra(p, min_reg.minimizer & mask1,
                              strand_buff[strand_pos]);
                same_min_count_w--;
            }
        }
        buff_pos = (buff_pos == w - 1) ? 0 : buff_pos + 1;
    }

    if (min_reg.minimizer != UINT64_MAX && !min_saved) {
        push_min_stra(p, min_reg.minimizer & mask1, min_reg.strand);
    }
}

void push_min_stra(min_stra_v *p, uint32_t min, uint8_t stra) {
    if (p->n == 0) {
        p->a[0].minimizer = min;
        p->a[0].strand = stra;
        p->repetition[0] = 1;
        p->n++;
        return;
    }
    for (size_t i = 0; i < p->n; i++) {
        if (p->a[i].minimizer == min && p->a[i].strand == stra) {
            p->repetition[i]++;
            return;
        }
    }
    p->a[p->n].minimizer = min;
    p->a[p->n].strand = stra;
    p->repetition[p->n] = 1;
    p->n++;
}
