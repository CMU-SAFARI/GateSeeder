#include "extraction.h"
#include <stddef.h>

void extract_minimizers(const base_t *read, min_stra_v *p) {}

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
