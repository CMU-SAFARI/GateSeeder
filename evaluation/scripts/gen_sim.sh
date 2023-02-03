#!/bin/bash

# Generate the reads

if [ $1 = 'ont' ]; then
	pbsim --strategy wgs --method errhmm --errhmm $DATA/ERRHMM-ONT.model --depth 0.2 --genome $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta --seed 23 --length-min 10000 --length-max 100000
	rm *.fastq
	rm *.ref
	paftools.js pbsim2fq $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta.gz.fai *.maf > sim.fasta
	rm *.maf
elif [ $1 = 'hifi' ]; then
	pbsim --strategy wgs --method errhmm --errhmm $DATA/ERRHMM-SEQUEL.model --depth 0.2 --genome $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta --seed 23 --length-min 12000 --length-max 24000
	rm *.fastq
	rm *.ref
	paftools.js pbsim2fq $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta.gz.fai *.maf > sim.fasta
	rm *.maf
elif [ $1 = 'illumina' ]; then
	mason_variator -ir $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta -s 3 -ov $DATA/hs38.vcf --snp-rate 1e-3 --small-indel-rate 2e-4 --sv-indel-rate 0 --sv-inversion-rate 0 --sv-translocation-rate 0 --sv-duplication-rate 0 --max-small-indel-size 10
	mason_simulator -ir $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta -iv $DATA/hs38.vcf -n 10000000 --seed 3 -o mason.fq -oa mason_illumina.sam --illumina-prob-mismatch-scale 2.5 --illumina-read-length 150
	paftools.js mason2fq mason_illumina.sam > sim.fasta
	rm *.sam *.fq $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta.fai
else
	exit 1
fi
