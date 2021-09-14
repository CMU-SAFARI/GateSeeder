#include "index.h"
#include "stdio.h"

#define FILE_NAME "res/GCF_000001405.26_GRCh38_genomic.fna"
//#define FILE_NAME "res/test3.fna"

int main() {
    create_index(FILE_NAME, 11, 14);
    return 0;
}
