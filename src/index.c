#include "index.h"
#include "mmpriv.h"
#include <stdlib.h>

index_t * index_init(uint32_t b) {
    index_t * idx = (index_t *) malloc(sizeof(index_t));
    idx->b = b;
    idx->bucket = (bucket_t *) calloc(1<<b, sizeof(bucket_t));
    return idx;
}

index_t * create_index(uint32_t b) {
    index_t * idx = index_init(b);
    return idx;
}
