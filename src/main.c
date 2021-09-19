#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FILE_NAME "res/GCF_000001405.26_GRCh38_genomic.fna"

int main(int argc, char *argv[]) {
    unsigned int w = 10;
    unsigned int k = 15;

    int option;
    while ((option = getopt(argc, argv, ":w:k:")) != -1) {
        switch (option) {
        case 'w':
            w = atoi(optarg);
            break;
        case 'k':
            k = atoi(optarg);
            break;
        case ':':
            fprintf(stderr, "Error: '%c' requires a value\n", optopt);
            exit(3);
        case '?':
            fprintf(stderr, "Error: unknown option: '%c'\n", optopt);
            exit(3);
        }
    }

    if (optind + 1 >= argc) {
        fputs("Error: expected genome reference file name & output file name\n",
              stderr);
        exit(3);
    }

    FILE *in_fp = fopen(argv[optind], "r");
    if (in_fp == NULL) {
        fprintf(stderr, "Error: cannot open `%s`\n", argv[optind]);
        exit(1);
    }

    FILE *out_fp = fopen(argv[optind + 1], "wb");
    if (out_fp == NULL) {
        fprintf(stderr, "Error: cannot open `%s`\n", argv[optind + 1]);
        exit(1);
    }

    index_t *idx = create_index(in_fp, w, k);
    fwrite(&(idx->n), sizeof(idx->n), 1, out_fp);
    fwrite(&(idx->m), sizeof(idx->m), 1, out_fp);
    fwrite(idx->h, sizeof(idx->h[0]), idx->n, out_fp);
    fwrite(idx->position, sizeof(idx->position[0]), idx->m, out_fp);
    fwrite(idx->strand, sizeof(idx->strand[0]), idx->m, out_fp);
    fclose(out_fp);
    printf("Info: Binary file `%s` written\n", argv[optind + 1]);
    return 0;
}
