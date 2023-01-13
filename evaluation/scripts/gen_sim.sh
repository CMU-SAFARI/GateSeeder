#!/bin/bash

# Generate the reads
#pbsim --prefix pb-1 --depth 0.1 --sample-fastq $DATA/m131017_060208_42213_c100579642550000001823095604021496_s1_p0.1.subreads.fastq --length-min 1000 --length-max 30000 --seed 11 $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta

pbsim --strategy wgs --method errhmm --errhmm $DATA/ERRHMM-ONT.model --depth 0.2 --genome $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta --seed 23 --length-min 10000 --length-max 100000

rm *.fastq
rm *.ref

# Convert maf to fastq
paftools.js pbsim2fq $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta.gz.fai *.maf > sim.fastq

rm *.maf
