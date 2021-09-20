#include "index.h"
#include "kvec.h"
#include "mmpriv.h"
#include <stdlib.h>
#define BUFFER_SIZE 4294967296

static inline unsigned char compare(mm72_t left, mm72_t right) {
    return (left.minimizer) <= (right.minimizer);
}

index_t *create_index(FILE *in_fp, const unsigned int w, const unsigned int k, const unsigned int filter_threshold) {
    printf("Info: w = %u & k = %u & f = %u\n", w, k, f);

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

    fread(read_buffer, sizeof(char), BUFFER_SIZE, in_fp);
    if (!feof(in_fp)) {
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
    fclose(in_fp);
    if (chromo_len > 0) {
        mm_sketch(0, dna_buffer, chromo_len, w, k, 0, 0, p);
        dna_len += chromo_len;
    }

    uint32_t max_mini = 0;
    for (unsigned int i = 0; i < p->n; i++) {
        if (max_mini < p->a[i].minimizer) {
            max_mini = p->a[i].minimizer;
        }
    }

    printf("Info: Indexed DNA length: %u bases\n", dna_len);
    printf("Info: Number of (minimizer, position, strand): %lu\n", p->n);
    printf("Info: Maximum minimizer: %u\n", max_mini);
    float strand_size = (float)p->n / (1 << 30);
    float position_size = strand_size * 4;
    float hash_size = (float)(max_mini + 1) / (1 << 28);
    printf("Info: Size of the position array: %fGB\n", position_size);
    printf("Info: Size of the strand array: %fGB\n", strand_size);
    printf("Info: Size of the minimizer array: %fGB\n", hash_size);
    printf("Info: Total size: %fGB\n", position_size + strand_size + hash_size);
    free(dna_buffer);

    merge_sort(p->a, 0, p->n - 1);

    printf("Info: Array sorted\n");

    // Write the data in the struct & filter out the most frequent minimizers
    uint32_t *h = (uint32_t *)malloc(sizeof(uint32_t) * (max_mini + 1));
    uint32_t *position = (uint32_t *)malloc(sizeof(uint32_t) * p->n);
    uint8_t *strand = (uint8_t *)malloc(sizeof(uint8_t) * p->n);
    if (h == NULL || position == NULL || strand == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    unsigned int diff_counter = 0;
    unsigned int index = p->a[0].minimizer;
    unsigned int freq_counter = 0;
    unsigned int filter_counter = 0;
    unsigned int pos = 0;
    unsigned int l = 0;
    for (unsigned int i = 1; i < p->n; i++) {
        if(index == p->a[i].minimizer) {
            freq_counter++;
        } else {
            if (freq_counter < filter_threshold) {
                diff_counter++;
                for (unsigned int j = pos; j < i; j++) {
                    position[l] = p->a[j].position;
                    strand[l] = p->a[j].strand;
                    l++;
                }
            } else {
                filter_counter++;
            }
            pos = i;
            freq_counter = 0;
            while (index != p->a[i].minimizer) {
                h[index] = l;
                index++;
            }
        }
    }
    if (freq_counter < filter_threshold) {
        diff_counter++;
        for (unsigned int j = pos; j < i; j++) {
            position[l] = p->a[j].position;
            strand[l] = p->a[j].strand;
            l++;
        }
    } else {
        filter_counter++;
    }
    h[index] = l;
    kv_destroy(*p);
    printf("Info: %u minimizer(s) ignored\n", filter_counter);
    printf("Info: Number of distinct minimizers = %u\n", diff_counter);
    index_t *idx = (index_t *)malloc(sizeof(index_t));
    idx->n = max_mini+1;
    idx->m = l;
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
