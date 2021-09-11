#include "index.h"
#include "mmpriv.h"
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 4294967296

index_t *create_index(const char *file_name, const unsigned int w,
                      const unsigned int k) {
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

    while (1) {
        char c = read_buffer[i];

        if (c == '>') {
            while (c != '\n' && c != 0) {
                i++;
                c = read_buffer[i];
            }
        } else {
            while (c != '\n' && c != 0) {
                dna_buffer[dna_len] = c;
                dna_len++;
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

    printf("Info: Indexed DNA length: %u bases\n", dna_len);

    mm128_v *p = (mm128_v *)calloc(1, sizeof(mm128_v));

    printf("Info: w = %u & k = %u\n", w, k);
    mm_sketch(0, dna_buffer, dna_len, w, k, 0, 0, p);

    printf("Info: Number of positions: %lu\n", p->n);
    float strand_size = (float)p->n / (1<<30);
    float position_size = strand_size * 4;
    float hash_size = (float)(1 << 2*(k+1)) / (1<<30);
    printf("Info: Size of the position array: %fGB\n", position_size);
    printf("Info: Size of the strand array: %fGB\n", strand_size);
    printf("Info: Size of the hash table: %fGB\n", hash_size);
    printf("Info: Total size: %fGB\n", position_size + strand_size + hash_size);

    free(dna_buffer);
    free(p);
    index_t *idx = (index_t *)malloc(sizeof(index_t));
    return idx;
}
