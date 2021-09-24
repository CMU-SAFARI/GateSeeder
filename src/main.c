#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    unsigned int w = 10;
    unsigned int k = 18;
    unsigned int f = 500;
    char p = 0;
    uint32_t b = 28;

    int option;
    while ((option = getopt(argc, argv, ":w:k:f:pb:")) != -1) {
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
        case 'b':
            b = atoi(optarg);
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

    index_t *idx = create_index(in_fp, w, k, f, b);
    fwrite(&(idx->n), sizeof(idx->n), 1, out_fp);
    fwrite(idx->h, sizeof(idx->h[0]), idx->n, out_fp);
    fwrite(idx->position, sizeof(idx->position[0]), idx->m, out_fp);
    fwrite(idx->strand, sizeof(idx->strand[0]), idx->m, out_fp);
    fclose(out_fp);
    printf("Info: Binary file `%s` written\n", argv[optind + 1]);

    if (p) {
        FILE *gnuplot = popen("gnuplot", "w");
        fprintf(gnuplot, "set terminal png size 1200, 900\n");
        // fprintf(gnuplot, "set logscale y\n");
        fprintf(gnuplot, "set output 'cumulative.png'\n");
        fprintf(gnuplot,
                "set title 'Cumulative sum of the number of entries "
                "(k = %u, w = %u, f = %u and b = %u)'\n",
                k, w, f, b);
        fprintf(gnuplot, "set xlabel 'Minimizers'\n");
        fprintf(gnuplot,
                "set ylabel 'Cumulative sum of the number of positions'\n");
        fprintf(gnuplot, "plot '-' with lines lw 3 notitle\n");
        for (unsigned int i = 0; i < idx->n; i += idx->n / 1000) {
            fprintf(gnuplot, "%u %u\n", i, idx->h[i]);
        }
        fprintf(gnuplot, "e\n");
        fflush(gnuplot);
        pclose(gnuplot);

        /*
        gnuplot = popen("gnuplot", "w");
        fprintf(gnuplot, "set terminal png size 1200, 900\n");
        fprintf(gnuplot, "set output 'cumulative1.png'\n");
        fprintf(gnuplot,
                "set title 'Cumulative sum of the number of entries "
                "starting at 6E8 (k = %u, w = %u, f = %u and b = %u)'\n",
                k, w, f, b);
        fprintf(gnuplot, "set xlabel 'Minimizers'\n");
        fprintf(gnuplot,
                "set ylabel 'Cumulative sum of the number of positions'\n");
        fprintf(gnuplot, "plot '-' with lines lw 3 notitle\n");
        unsigned int i;
        for (i = 600000000; i < idx->n; i += idx->n / 10000) {
            fprintf(gnuplot, "%u %u\n", i, idx->h[i] - idx->h[600000000]);
        }
        fprintf(gnuplot, "e\n");
        fflush(gnuplot);
        pclose(gnuplot);

        gnuplot = popen("gnuplot", "w");
        fprintf(gnuplot, "set terminal png size 1200, 900\n");
        fprintf(gnuplot, "set output 'cumulative2.png'\n");
        fprintf(gnuplot,
                "set title 'Cumulative sum of the number of entries "
                "starting at 8.5E8 (k = %u, w = %u, f = %u and b = %u)'\n",
                k, w, f, b);
        fprintf(gnuplot, "set xlabel 'Minimizers'\n");
        fprintf(gnuplot,
                "set ylabel 'Cumulative sum of the number of positions'\n");
        fprintf(gnuplot, "plot '-' with lines lw 3 notitle\n");
        for (i = 850000000; i < idx->n; i += idx->n / 10000) {
            fprintf(gnuplot, "%u %u\n", i, idx->h[i] - idx->h[850000000]);
        }
        fprintf(gnuplot, "e\n");
        fflush(gnuplot);
        pclose(gnuplot);

        puts("Info: cumulative.png written");
        */
    }
    return 0;
}
