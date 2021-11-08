#ifndef PARSE_H
#define PARSE_H

#include "compare.h"
#include "seeding.h"
#include <stdio.h>

void parse_paf(FILE *fp, target_v *target);
void parse_fastq(FILE *fp, read_v *reads);

#endif
