#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FILE_NAME "res/GCF_000001405.26_GRCh38_genomic.fna"

int main(int argc, char *argv[]) {
    unsigned int w = 10;
    unsigned int k = 15;
    unsigned int f = 500;
    char p = 0;

    int option;
    while ((option = getopt(argc, argv, ":w:k:f:p")) != -1) {
        switch (option) {
        case 'w':
            w = atoi(optarg);
            break;
        case 'k':
            k = atoi(optarg);
            break;
        case 'f':
            f = atoi(optarg);
            break;
        case 'p':
            p = 1;
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

    index_t *idx = create_index(in_fp, w, k, f);
    fwrite(&(idx->n), sizeof(idx->n), 1, out_fp);
    fwrite(idx->h, sizeof(idx->h[0]), idx->n, out_fp);
    fwrite(idx->position, sizeof(idx->position[0]), idx->m, out_fp);
    fwrite(idx->strand, sizeof(idx->strand[0]), idx->m, out_fp);
    fclose(out_fp);
    printf("Info: Binary file `%s` written\n", argv[optind + 1]);

    if (p) {
        FILE *gnuplot = popen("gnuplot", "w");
        fprintf(gnuplot, "set terminal png size 2000,1500\n");
        fprintf(gnuplot, "set output 'plot.png'\n");
        fprintf(gnuplot, "plot '-'\n");
        for (unsigned int i = 0; i < idx->n; i += idx->n / 1000) {
            fprintf(gnuplot, "%u %u\n", i, idx->h[i]);
        }
        fprintf(gnuplot, "e\n");
        fflush(gnuplot);
        puts("Info: plot.png written\n");
    }
    return 0;
}
