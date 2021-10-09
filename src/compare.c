#include "compare.h"
#include "query.h"
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

        for (unsigned char j = 0; j < 3; j++) {
            while (c != '\t') {
                i++;
                c = read_buffer[i];
            }
            i++;
            c = read_buffer[i];
        }

        number_i = 0;
        while (c != '\t') {
            number_buffer[number_i] = c;
            number_i++;
            i++;
            c = read_buffer[i];
        }
        number_buffer[number_i] = '\0';
        loc_buffer.a[loc_buffer.n - 1].quality =
            strtoul(number_buffer, NULL, 10);
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

void compare(target_v tar, read_v reads, index_t idx, const size_t len,
             const unsigned int w, const unsigned int k, const unsigned int b,
             const unsigned int min_t, const unsigned int loc_r) {
    unsigned int tp_counter = 0;
    unsigned int um_counter = 0;
    unsigned int m_counter = 0;
    unsigned int tn_counter = 0;
    unsigned int loc_counter = 0;
    unsigned int quality_counter_tn = 0;
    unsigned int quality_counter_tp = 0;
    location_v locs;
    char flag = 0;
    char buff[50000] = {0};
    size_t j = 0;
    for (size_t i = 0; i < reads.n; i++) {
        //if (strcmp(reads.name[i], "ERR240727.48861\0") == 0) {
            //printf("%s\n", reads.name[i]);
            get_locations(idx, reads.a[i], len, w, k, b, min_t, loc_r, &locs);
        //}
        if (j < tar.n) {
            if (strcmp(tar.a[j].name, reads.name[i]) == 0) {
                loc_counter += tar.a[j].n;
                for (size_t k = 0; k < locs.n; k++) {
                    flag = 0;
                    for (size_t l = 0; l < tar.a[j].n; l++) {
                        if (locs.a[k] >= tar.a[j].a[l].start &&
                            locs.a[k] <= tar.a[j].a[l].end) {
                            tp_counter++;
                            quality_counter_tp += tar.a[j].a[l].quality;
                            flag = 1;
                            buff[l] = 1;
                        }
                    }
                    if (!flag) {
                        um_counter++;
                    } else {
                        m_counter++;
                    }
                }
                for (size_t l = 0; l < tar.a[j].n; l++) {
                    if (buff[l]) {
                        buff[l] = 0;
                    } else {
                        tn_counter++;
                        quality_counter_tn += tar.a[j].a[l].quality;
                    }
                }
                j++;
            } else {
                um_counter += locs.n;
            }
        } else {
            um_counter += locs.n;
        }
    }
    printf("Info: Number of true positives %u (%f%%)\n", tp_counter,
           ((float)loc_counter - tn_counter) / loc_counter * 100);
    printf("Info: Average mapping quality of the true positives %u\n",
           quality_counter_tp / tp_counter);
    printf("Info: Number of true negatives %u (%f%%)\n", tn_counter,
           ((float)tn_counter) / loc_counter * 100);
    printf("Info: Average mapping quality of the true negatives %u\n",
           quality_counter_tn / tn_counter);
    printf("Info: Number of unmatching locations %u\n", um_counter);
    printf("Info: Number of found locations %u\n", um_counter + m_counter);
    printf("Info: Percentage of matching locations compared to the found "
           "locations "
           "%f%%\n",
           ((float)m_counter) / (um_counter + m_counter) * 100);
}
