#ifndef INDEX_H
#define INDEX_H

#include <stdint.h>

typedef struct {
    uint8_t *strand;     // strand array
    uint32_t *poisition; // position array
    uint32_t *h;         // hash table // can be an issue, maybe on 64 bits ???
} index_t;

index_t *create_index(const char *file_name, const unsigned int w,
                      const unsigned int k);

#endif
