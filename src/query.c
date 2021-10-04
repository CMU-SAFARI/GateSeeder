#include "query.h"
#include "kvec.h"
#include "mmpriv.h"
#include <assert.h>
#include <stdlib.h>
#define BUFFER_SIZE 4294967296
#define LOCATION_RANGE 200
#define LOCATION_BUFFER_SIZE 50000
#define MINIMIZER_THRESHOLD 6

static inline int cmp(const void *p1, const void *p2) {
    buffer_t *l1 = (buffer_t *)p1;
    buffer_t *l2 = (buffer_t *)p2;
    return l1->location - l2->location;
}

void parse_fastq(FILE *fp, read_v *reads) {
    char *read_buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if (read_buffer == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    fread(read_buffer, sizeof(char), BUFFER_SIZE, fp);
    if (!feof(fp)) {
        fputs("Reading error: buffer too small\n", stderr);
        fclose(fp);
        exit(3);
    }

    reads->n = 0;
    reads->a = NULL;

    size_t i = 0;

    for (;;) {
        char c = read_buffer[i];

        if (c == '@') {
            while (c != '\n') {
                i++;
                c = read_buffer[i];
            }
            reads->n++;
            reads->a = (char **)realloc(reads->a, reads->n * sizeof(char *));
            if (reads->a == NULL) {
                fputs("Memory error\n", stderr);
                exit(2);
            }
            reads->a[reads->n - 1] =
                (char *)malloc(READ_LENGTH * sizeof(char *));
            if (reads->a[reads->n - 1] == NULL) {
                fputs("Memory error\n", stderr);
                exit(2);
            }
            for (size_t j = 0; j < READ_LENGTH; j++) {
                i++;
                reads->a[reads->n - 1][j] = read_buffer[i];
            }
            i++;
            assert(read_buffer[i] == '\n');
            i++;
            assert(read_buffer[i] == '+');
            i = READ_LENGTH + i;
        } else if (c == 0) {
            break;
        }
        i++;
    }
    free(read_buffer);
    fclose(fp);
    return;
}

void get_locations(index_t idx, char *read, const size_t len,
                   const unsigned int w, const unsigned int k,
                   const unsigned int b, location_v *locs) {
    mm72_v p = {.n = 0, .m = 0, .a = NULL};
    // TODO We only need the minimizers and the strand, can be optimized
    mm_sketch(0, read, READ_LENGTH, w, k, b, 0, &p);

    buffer_t buffer[LOCATION_BUFFER_SIZE];
    size_t n = 0;
    uint32_t min, max;
    uint32_t minimizer;
    for (size_t i = 0; i < p.n; i++) {
        minimizer = p.a[i].minimizer;
        min = (minimizer == 0) ? 0 : idx.h[minimizer - 1];
        max = idx.h[minimizer];

        for (uint32_t j = min; j < max; j++) {
            buffer[n] = (buffer_t){.location = idx.location[j],
                                   .strand = idx.strand[j] ^ p.a[i].strand};
            n++;
        }
    }
    kv_destroy(p);

    qsort(buffer, n, sizeof(buffer_t), cmp);

    uint32_t loc_buffer[LOCATION_BUFFER_SIZE];
    locs->n = 0;
    size_t loc_counter = 1;
    size_t init_loc_idx = 0;
    while (init_loc_idx < n - MINIMIZER_THRESHOLD + 1 &&
           loc_counter + init_loc_idx < n) {
        if ((buffer[init_loc_idx + loc_counter].location -
                 buffer[init_loc_idx].location <
             LOCATION_RANGE) &&
            buffer[init_loc_idx + loc_counter].strand ==
                buffer[init_loc_idx].strand) {
            loc_counter++;
        } else {
            if (loc_counter >= MINIMIZER_THRESHOLD) {
                loc_buffer[locs->n] = buffer[init_loc_idx].location;
                locs->n++;
                init_loc_idx += loc_counter;
            } else {
                init_loc_idx++;
            }
            loc_counter = 1;
        }
    }

    if (loc_counter >= MINIMIZER_THRESHOLD) {
        loc_buffer[locs->n] = buffer[init_loc_idx].location;
        locs->n++;
        init_loc_idx += loc_counter;
    }
    locs->a = (uint32_t *)malloc(locs->n * sizeof(uint32_t));
    for (size_t i = 0; i < locs->n; i++) {
        locs->a[i] = loc_buffer[i];
    }
}
