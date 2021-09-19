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

    if (optind >= argc) {
        fputs("Error: expected genome reference file name\n", stderr);
        exit(3);
    }

    FILE *fp = fopen(argv[optind], "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: cannot open `%s`\n", argv[optind]);
        exit(1);
    }

    create_index(fp, w, k);
    return 0;
}
