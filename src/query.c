#include "query.h"
#include <stdlib.h>
#define READ_LENGTH 100
#define BUFFER_SIZE 4294967296

void query(index_t *idx, FILE *fp) {
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

    /*
    size_t i = 0;
    while (1) {
        char c = read_buffer[i];

        if (c == '@') {
            if (chromo_len > 0) {
                mm_sketch(0, dna_buffer, chromo_len, w, k, b, 0, p);
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
*/
    free(read_buffer);
    fclose(fp);
}

location_v *get_locations(index_t *idx, char *read, size_t length) {
    return NULL;
}
