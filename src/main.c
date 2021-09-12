#include "index.h"
#include "stdio.h"

#define FILE_NAME "res/GCF_000001405.26_GRCh38_genomic.fna"
//#define FILE_NAME "res/test3.fna"

int main() {
    index_t *idx = create_index(FILE_NAME, 10, 15);
    printf("%u\n", idx->h[0]);
    return 0;
}
