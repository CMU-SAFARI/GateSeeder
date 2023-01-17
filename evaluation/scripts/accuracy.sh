#!/bin/bash

RES=accuracy_demeter.txt
TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
QUERY=sim.fastq

if [ $1 = 'ont' ]; then
	XCLBIN=../device/demeter_ont.xclbin
	RANGE_MAX_OCC=$(eval echo "{10..100..10}")
	RANGE_VT_DISTANCE=$(eval echo "{500..1100..50}")
	W=10
	K=15
	MM2_PRESET='map-ont'
elif [ $1 = 'hifi']; then
	XCLBIN=../device/demeter_hifi.xclbin
	W=19
	K=19
	RANGE_VT_DISTANCE=$(eval echo "{500..1200..50}")
	MM2_PRESET='map-hifi'
else
	exit 1
fi

rm -f $RES

for max_occ in $RANGE_MAX_OCC
do
	echo "[ACC] Generating the index with max_occ: $max_occ"
	../demeter_index -t 32 -w $W -k $K -f $max_occ $TARGET $DATA/index.dti
	for vt_distance in $RANGE_VT_DISTANCE
	do
		echo "[ACC] Running demeter with vt_distance: $vt_distance"
		../demeter -t 32 -d $vt_distance $XCLBIN $DATA/index.dti $QUERY -o mapping.paf
		echo "[ACC] Evaluating the results"
		echo -e "P\t$max_occ\t$vt_distance" >> $RES
		paftools.js mapeval mapping.paf >> $RES
	done
done

echo "[ACC] Running minimap2"
minimap2 -t 32 -x $MM2_PRESET -o mapping.paf $TARGET $QUERY
paftools.js mapeval mapping.paf > accuracy_mm2.txt

rm -f index.dti
rm -f mapping.paf
