#include "extraction.h"
#include <stddef.h>

void extract_minimizers(const base_t *read, min_stra_v *p) {
    min_stra_t buff[W];

    ap_uint<READ_LEN_LOG> l(0); // l counts the number of bases and is reset to
                                // 0 each time there is an ambiguous base
    ap_uint<W_LOG> buff_pos(0);
    ap_uint<W_LOG> min_pos(0);
    min_stra_t min_reg = {~ap_uint<2 * K>(0), 0};
    ap_uint<1> min_saved(0);
    ap_uint<W_LOG> same_min_count(0);
    ap_uint<1> strand_buff[W];
    ap_uint<W_LOG> strand_pos(0);
    // HERE
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
            unsigned char z = kmer[0] < kmer[1] ? 0 : 1; // strand
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

void push_min_stra(min_stra_v *p, min_stra_t val) {
    min_stra_b_t min_stra = {val.minimizer, val.strand};
    if (p->n == 0) {
        p->a[0] = min_stra;
        p->repetition[0] = 1;
        p->n++;
        return;
    }
LOOP_push_min_stra:
    for (size_t i = 0; i < p->n; i++) {
        if (p->a[i].minimizer == min_stra.minimizer &&
            p->a[i].strand == min_stra.strand) {
            p->repetition[i]++;
            return;
        }
    }
    p->a[p->n] = min_stra;
    p->repetition[p->n] = 1;
    p->n++;
}
