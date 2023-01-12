#!/bin/bash

make -C ..
rm -f accuracy.txt
for max_occ in {50..700..50}
do
	echo "[ACC] Generating the index with max_occ: $max_occ"
	echo "[ACC] ../demeter_index -t 32 -f $max_occ $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta index.dti"
	../demeter_index -t 32 -f $max_occ $DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta index.dti
	echo "[ACC] Index generated"
	for vt_distance in 10 20 50 100 300 500 1000
	do
		echo "[ACC] Running demeter with vt_distance: $vt_distance"
		echo "[ACC] ../demeter -t 32 -d $vt_distance -h 0.0,0 ../device/demeter.xclbin index.dti sim.fastq -o mapping.paf"
		../demeter -t 32 -d $vt_distance -h 0.0,0 ../device/demeter.xclbin index.dti sim.fastq -o mapping.paf
		echo "[ACC] paftools.js mapeval mapping.paf"
		echo -e "$max_occ\t$vt_distance" >> accuracy.txt
		paftools.js mapeval mapping.paf >> accuracy.txt
	done
done
