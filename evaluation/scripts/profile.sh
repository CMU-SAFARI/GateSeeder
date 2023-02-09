#!/bin/bash

TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta

if [ $1 = 'ont' ]; then
	XCLBIN=../device/demeter_ont.xclbin
	QUERY=$DATA/HG002_ONT-UL_GIAB_20200204_1000filtered_2Mreads.fasta
	RANGE_MAX_OCC=(10 50 100)
	RANGE_BATCH_SIZE=(80000000 40000000 10000000)
	W=10
	K=15
	MM2_PRESET='map-ont'
elif [ $1 = 'hifi' ]; then
	XCLBIN=../device/demeter_hifi.xclbin
	QUERY=$DATA/m64011_190830_220126.fasta
	W=19
	K=19
	RANGE_MAX_OCC=(1 2 5)
	RANGE_BATCH_SIZE=(268435456 268435456 268435456)
	MM2_PRESET='map-hifi'
elif [ $1 = 'illumina' ]; then
	XCLBIN=../device/demeter_illumina.xclbin
	QUERY=$DATA/illumina_90.fasta
	W=11
	K=21
	RANGE_MAX_OCC=$(eval echo "{10..100..10}")
	MM2_PRESET='sr'
else
	exit 1
fi

RES_DIR=profile_$1
mkdir -p $RES_DIR
rm -f $RES_DIR/*

for i in "${!RANGE_MAX_OCC[@]}";
do
	echo "[PROF] Generating the index with max_occ: ${RANGE_MAX_OCC[i]}"
	../seedfarm_index -t 32 -w $W -k $K -f ${RANGE_MAX_OCC[i]} $TARGET $DATA/index.sfi
	echo "[PROF] Locking the reads and the index into RAM"
	vmtouch -ldw $QUERY $DATA/index.sfi
	echo "[PROF] Running seedfarm"
	../seedfarm -b  ${RANGE_BATCH_SIZE[i]} -t 32 -s $XCLBIN $DATA/index.sfi $QUERY -o /dev/null > $RES_DIR/${RANGE_MAX_OCC[i]}.dat
done

rm -f $DATA/index.sfi
