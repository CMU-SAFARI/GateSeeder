#!/bin/bash
TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
QUERY=sim.fasta

if [ $1 = 'ont' ]; then
	W=10
	K=15
	RANGE_MAX_OCC="10 50 100"
	RANGE_VT_DISTANCE=$(eval echo "{500..1100..50}")
	MM2_PRESET='map-ont'
	nb_reads=$(expr $(wc -l < $QUERY) / 2)
elif [ $1 = 'hifi' ]; then
	W=19
	K=19
	RANGE_MAX_OCC="1 2 5"
	RANGE_VT_DISTANCE=$(eval echo "{500..5000..250}")
	MM2_PRESET='map-hifi'
	nb_reads=$(expr $(wc -l < $QUERY) / 2)
elif [ $1 = 'illumina' ]; then
	W=11
	K=21
	RANGE_MAX_OCC="500"
	RANGE_VT_DISTANCE=$(eval echo "{10..150..20}")
	MM2_PRESET='sr'
	nb_reads=$(expr $(wc -l < $QUERY) / 4)
else
	exit 1
fi

XCLBIN=../device/demeter_$1.xclbin
RES_DIR=accuracy_$1
mkdir -p $RES_DIR
rm -f $RES_DIR/*

echo -e "$1\t$nb_reads" > $RES_DIR/info.dat

for max_occ in $RANGE_MAX_OCC
do
	echo "[ACC] Generating the index with max_occ: $max_occ"
	../demeter_index -t 32 -w $W -k $K -f $max_occ $TARGET $DATA/index.gfi
	for vt_distance in $RANGE_VT_DISTANCE
	do
		echo "[ACC] Running demeter with vt_distance: $vt_distance"
		../demeter -t 32 -d $vt_distance $XCLBIN $DATA/index.gfi $QUERY -o mapping.paf
		echo "[ACC] Evaluating the results"
		echo -e "max_occ: $max_occ, vt_dst: $vt_distance" > $RES_DIR/seedfarm_${max_occ}_${vt_distance}_$1.dat
		paftools.js mapeval mapping.paf >> $RES_DIR/seedfarm_${max_occ}_${vt_distance}_$1.dat
	done
done

echo "[ACC] Running minimap2"
minimap2 -t 32 -x $MM2_PRESET -o mapping.paf $TARGET $QUERY
echo "[ACC] Evaluating the results"
echo -e "minimap2" > $RES_DIR/minimap2_$1.dat
paftools.js mapeval mapping.paf >> $RES_DIR/minimap2_$1.dat

rm -f $DATA/index.gfi
rm -f mapping.paf
