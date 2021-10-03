#include "query.h"
#include "mmpriv.h"
#include <assert.h>
#include <stdlib.h>
#define BUFFER_SIZE 4294967296
#define LOCATION_RANGE 200
#define LOCATION_BUFFER_SIZE 5000
#define MINIMIZER_THRESHOLD 7

static inline int cmp(const void *p1, const void *p2) {
    buffer_t *l1 = (buffer_t *)p1;
    buffer_t *l2 = (buffer_t *)p2;
    return l1->location - l2->location;
}

read_v *parse_fastq(FILE *fp) {
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

    read_v *reads = (read_v *)calloc(1, sizeof(read_v));
    if (reads == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }
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
    return reads;
}

location_v *get_locations(index_t *idx, char *read, const size_t len,
                          const unsigned int w, const unsigned int k,
                          const unsigned int b) {
    mm72_v *p = (mm72_v *)calloc(1, sizeof(mm72_v));
    if (p == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }
    // TODO We only need the minimizers and the strand, can be optimized
    mm_sketch(0, read, READ_LENGTH, w, k, b, 0, p);

    buffer_t buffer[LOCATION_BUFFER_SIZE];
    size_t n = 0;
    uint32_t min, max;
    uint32_t minimizer;
    for (size_t i = 0; i < p->n; i++) {
        minimizer = p->a[i].minimizer;
        min = (minimizer == 0) ? 0 : idx->h[minimizer - 1];
        max = idx->h[minimizer];

        for (uint32_t j = min; j < max; j++) {
            buffer[n] = (buffer_t){.location = idx->location[j],
                                   .strand = idx->strand[j] ^ p->a[i].strand};
            n++;
        }
    }

    qsort(buffer, n, sizeof(buffer_t), cmp);

    uint32_t loc_buffer[LOCATION_BUFFER_SIZE];
    location_v *locations = (location_v *)calloc(1, sizeof(location_v));
    if (p == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    unsigned char loc_counter = 1;
    size_t init_loc_idx = 0;
    while (init_loc_idx != n - MINIMIZER_THRESHOLD + 2) {
        if ((buffer[init_loc_idx + loc_counter].location -
                 buffer[init_loc_idx].location <
             LOCATION_RANGE) &&
            buffer[init_loc_idx + loc_counter].strand ==
                buffer[init_loc_idx].strand) {
            loc_counter++;
        } else {
            if (loc_counter >= MINIMIZER_THRESHOLD) {
                loc_buffer[locations->n] = buffer[init_loc_idx].location;
                locations->n++;
                init_loc_idx += loc_counter;
            } else {
                init_loc_idx++;
            }
            loc_counter = 1;
        }
    }
    locations->a = (uint32_t *)malloc(locations->n * sizeof(uint32_t));
    for (size_t i = 0; i < locations->n; i++) {
        locations->a[i] = loc_buffer[i];
    }
    return locations;
}
