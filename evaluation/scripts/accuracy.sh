#!/bin/bash

RES=accuracy_demeter.txt
TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
QUERY=sim.fastq
XCLBIN=../device/demeter.xclbin

rm -f $RES

#for max_occ in {50..700..50}
for max_occ in {10..40..10}
do
	echo "[ACC] Generating the index with max_occ: $max_occ"
	../demeter_index -t 32 -f $max_occ $TARGET $DATA/index.dti
	for vt_distance in {500..1200..50}
	do
		echo "[ACC] Running demeter with vt_distance: $vt_distance"
		../demeter -t 32 -d $vt_distance $XCLBIN $DATA/index.dti $QUERY -o mapping.paf
		echo "[ACC] Evaluating the results"
		echo -e "P\t$max_occ\t$vt_distance" >> $RES
		paftools.js mapeval mapping.paf >> $RES
	done
done

echo "[ACC] Running minimap2"
minimap2 -t 32 -x map-ont -o mapping.paf $TARGET $QUERY
paftools.js mapeval mapping.paf > accuracy_mm2.txt

rm -f index.dti
rm -f mapping.paf
