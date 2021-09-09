#ifndef INDEX_H
#define INDEX_H

#include <stdint.h>

typedef struct {
    uint32_t len;
    uint64_t *y;
} bucket_t;

typedef struct {
    uint32_t b; // 1<<b: number of buckets
    bucket_t *bucket;
} index_t;

index_t * index_init(uint32_t b);
index_t * create_index(uint32_t b);

#endif
