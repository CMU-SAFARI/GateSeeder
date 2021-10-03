#include "query.h"
#include <assert.h>
#include <stdlib.h>
#define BUFFER_SIZE 4294967296
#define LOCATION_RANGE 200
#define MINIMIZER_THRESHOLD 7

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

location_v *get_locations(index_t *idx, char *read, size_t length) {
    for (size_t i = 0; i < READ_LENGTH; i++) {
        printf("%c", read[i]);
    }
    printf("\n");
    return NULL;
}
