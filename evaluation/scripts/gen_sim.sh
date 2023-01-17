#!/bin/bash

# Generate the reads

if [ $1 = 'ont' ]; then
	pbsim --strategy wgs --method errhmm --errhmm $DATA/ERRHMM-ONT.model --depth 0.2 --genome $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta --seed 23 --length-min 10000 --length-max 100000
elif [ $1 = 'hifi' ]; then
	pbsim --strategy wgs --method errhmm --errhmm $DATA/ERRHMM-SEQUEL.model --depth 0.2 --genome $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta --seed 23 --length-min 10000 --length-max 100000
else
	exit 1
fi

rm *.fastq
rm *.ref

# Convert maf to fastq
paftools.js pbsim2fq $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta.gz.fai *.maf > sim.fastq

rm *.maf
