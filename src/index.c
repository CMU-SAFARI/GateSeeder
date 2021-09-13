#include "index.h"
#include "kvec.h"
#include "mmpriv.h"
#include <stdio.h>
#include <stdlib.h>
#define BUFFER_SIZE 4294967296

static inline unsigned char compare(mm72_t left, mm72_t right) {
    return (left.minimizer) <= (right.minimizer);
}

index_t *create_index(const char *file_name, const unsigned int w,
                      const unsigned int k) {
    printf("Info: w = %u & k = %u\n", w, k);

    FILE *fp = fopen(file_name, "r");
    if (fp == NULL) {
        fputs("File error\n", stderr);
        exit(1);
    }

    char *read_buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if (read_buffer == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    char *dna_buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if (dna_buffer == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    fread(read_buffer, sizeof(char), BUFFER_SIZE, fp);
    if (!feof(fp)) {
        fputs("Reading error: buffer too small\n", stderr);
        exit(3);
    }

    unsigned int i = 0;
    unsigned int dna_len = 0;
    unsigned int chromo_len = 0;

    mm72_v *p = (mm72_v *)calloc(1, sizeof(mm72_v));

    while (1) {
        char c = read_buffer[i];

        if (c == '>') {
            if (chromo_len > 0) {
                mm_sketch(0, dna_buffer, chromo_len, w, k, 0, 0, p);
                dna_len += chromo_len;
                chromo_len = 0;
            }
            while (c != '\n' && c != 0) {
                i++;
                c = read_buffer[i];
            }
        } else {
            while (c != '\n' && c != 0) {
                dna_buffer[chromo_len] = c;
                chromo_len++;
                i++;
                c = read_buffer[i];
            }
        }

        if (c == 0) {
            break;
        }
        i++;
    }

    free(read_buffer);
    fclose(fp);
    if (chromo_len > 0) {
        mm_sketch(0, dna_buffer, chromo_len, w, k, 0, 0, p);
        dna_len += chromo_len;
    }

    printf("Info: Indexed DNA length: %u bases\n", dna_len);
    printf("Info: Number of positions: %lu\n", p->n);
    float strand_size = (float)p->n / (1 << 30);
    float position_size = strand_size * 4;
    float hash_size = k >= 14 ? 1 << (2 * (k + 1) - 30)
                              : 1 / (float)(1 << (30 - 2 * (k + 1)));
    printf("Info: Size of the position array: %fGB\n", position_size);
    printf("Info: Size of the strand array: %fGB\n", strand_size);
    printf("Info: Size of the hash table: %fGB\n", hash_size);
    printf("Info: Total size: %fGB\n", position_size + strand_size + hash_size);
    free(dna_buffer);

    merge_sort(p->a, 0, p->n - 1);

    printf("Info: Array sorted\n");

    uint32_t *h = (uint32_t *)malloc(sizeof(uint32_t) * (1 << (2 * k)));
    uint32_t *position = (uint32_t *)malloc(sizeof(uint32_t) * p->n);
    uint8_t *strand = (uint8_t *)malloc(sizeof(uint8_t) * p->n);
    if (h == NULL || position == NULL || strand == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    unsigned int index = p->a[0].minimizer;
    char diff = 0;
    unsigned int diff_c = 0;
    for (unsigned int i = 0; i < p->n; i++) {
        position[i] = p->a[i].position;
        strand[i] = p->a[i].strand;
        while (index != p->a[i].minimizer) {
            if (!diff) {
                diff_c++;
            }
            diff = 1;
            h[index] = i;
            index++;
        }
        diff = 0;
    }
    kv_destroy(*p);
    printf("Info: Distinct minimizers = %u\n", diff_c + 1);
    index_t *idx = (index_t *)malloc(sizeof(index_t));
    if (idx == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }
    idx->h = h;
    idx->position = position;
    idx->strand = strand;
    return idx;
}

void merge_sort(mm72_t *a, unsigned int l, unsigned int r) {
    if (l < r) {
        unsigned int m = l + (r - l) / 2;
        merge_sort(a, l, m);
        merge_sort(a, m + 1, r);
        merge(a, l, m, r);
    }
}

void merge(mm72_t *a, unsigned int l, unsigned int m, unsigned int r) {
    unsigned int i, j;
    unsigned int n1 = m - l + 1;
    unsigned int n2 = r - m;

    mm72_t *L = (mm72_t *)malloc(n1 * sizeof(mm72_t));
    mm72_t *R = (mm72_t *)malloc(n2 * sizeof(mm72_t));
    if (L == NULL || R == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    for (i = 0; i < n1; i++) {
        L[i] = a[l + i];
    }
    for (j = 0; j < n2; j++) {
        R[j] = a[m + 1 + j];
    }

    i = 0;
    j = 0;
    unsigned int k = l;

    while (i < n1 && j < n2) {
        if (compare(L[i], R[j])) {
            a[k] = L[i];
            i++;
        } else {
            a[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        a[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        a[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}
