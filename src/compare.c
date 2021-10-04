#include "compare.h"
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 4294967296

void parse_paf(FILE *fp, target_v *target) {
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

    target->n = 0;
    target->a = NULL;

    range_t range_buffer[1000];
    t_location_v loc_buffer = {.n = 0, .a = range_buffer, .name = {'\0'}};

    char name_buffer[100] = {0};
    char number_buffer[100] = {0};
    size_t number_i = 0;
    unsigned char diff = 0;
    unsigned char first_line = 1;
    size_t i = 0;
    size_t name_i = 0;
    for (;;) {
        char c = read_buffer[i];
        if (c == 0) {
            break;
        }
        name_i = 0;
        diff = 0;
        while (c != '\t') {
            if (name_buffer[name_i] != c && diff == 0 && !first_line) {
                diff = 1;
                target->n++;
                target->a = (t_location_v *)realloc(
                    target->a, target->n * sizeof(t_location_v));
                if (target->a == NULL) {
                    fputs("Memory error\n", stderr);
                    exit(2);
                }
                target->a[target->n - 1].n = loc_buffer.n;
                target->a[target->n - 1].a =
                    (range_t *)malloc(sizeof(range_t) * loc_buffer.n);
                for (size_t j = 0; j < loc_buffer.n; j++) {
                    target->a[target->n - 1].a[j] = loc_buffer.a[j];
                }
                if (target->a[target->n - 1].a == NULL) {
                    fputs("Memory error\n", stderr);
                    exit(2);
                }
                strcpy(target->a[target->n - 1].name, name_buffer);
                loc_buffer.n = 0;
            }
            name_buffer[name_i] = c;
            i++;
            name_i++;
            c = read_buffer[i];
        }
        name_buffer[name_i] = '\0';
        first_line = 0;

        for (unsigned char j = 0; j < 7; j++) {
            while (c != '\t') {
                i++;
                c = read_buffer[i];
            }
            i++;
            c = read_buffer[i];
        }

        loc_buffer.n++;
        number_i = 0;
        while (c != '\t') {
            number_buffer[number_i] = c;
            number_i++;
            i++;
            c = read_buffer[i];
        }
        number_buffer[number_i] = '\0';
        loc_buffer.a[loc_buffer.n - 1].start = strtoul(number_buffer, NULL, 10);

        i++;
        c = read_buffer[i];
        number_i = 0;
        while (c != '\t') {
            number_buffer[number_i] = c;
            number_i++;
            i++;
            c = read_buffer[i];
        }
        number_buffer[number_i] = '\0';
        loc_buffer.a[loc_buffer.n - 1].end = strtoul(number_buffer, NULL, 10);

        while (c != '\n') {
            i++;
            c = read_buffer[i];
        }
        i++;
    }
    target->n++;
    target->a =
        (t_location_v *)realloc(target->a, target->n * sizeof(t_location_v));
    if (target->a == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }
    target->a[target->n - 1].n = loc_buffer.n;
    target->a[target->n - 1].a =
        (range_t *)malloc(sizeof(range_t) * loc_buffer.n);
    for (size_t j = 0; j < loc_buffer.n; j++) {
        target->a[target->n - 1].a[j] = loc_buffer.a[j];
    }
    if (target->a[target->n - 1].a == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }
    strcpy(target->a[target->n - 1].name, name_buffer);

    free(read_buffer);
    fclose(fp);
}

void compare(target_v tar, res_v res) {
    /*
       unsigned int tp_counter = 0;
       unsigned int fp_counter = 0;
       unsigned int tn_counter = 0;
       */
}
