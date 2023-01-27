#!/bin/bash

RES=performances_demeter.txt
TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta

if [ $1 = 'ont' ]; then
	XCLBIN=../device/demeter_ont.xclbin
	QUERY=$DATA/HG002_ONT-UL_GIAB_20200204_1000filtered_2Mreads.fasta
	#RANGE_MAX_OCC=$(eval echo "{10..100..10}")
	#RANGE_MAX_OCC=$(eval echo "10 50 100")
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
	RANGE_MAX_OCC=$(eval echo "{1..5..1}")
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


rm -f $RES
echo "[PERF] Loading the xclbin"
xbutil program -d 0000:c4:00.1 -u $XCLBIN

for i in "${!RANGE_MAX_OCC[@]}";
do
	echo "[PERF] Generating the index with max_occ: ${RANGE_MAX_OCC[i]}"
	../demeter_index -t 32 -w $W -k $K -f ${RANGE_MAX_OCC[i]} $TARGET $DATA/index.dti
	echo "[PERF] Locking the reads and the index into RAM"
	vmtouch -ldw $QUERY $DATA/index.dti
	echo "[PERF] Running demeter"
	start_date=`date +%s%N`
	../demeter -b  ${RANGE_BATCH_SIZE[i]} -t 32 $XCLBIN $DATA/index.dti $QUERY -o $DATA/mapping.paf
	end_date=`date +%s%N`
	echo `expr $end_date - $start_date` >> $RES
	pkill vmtouch
done

echo "[PERF] Generating the index for minimap2"
minimap2 -t 32 -x $MM2_PRESET -d $DATA/index.mmi $TARGET
echo "[PERF] Locking the reads and the index into RAM"
vmtouch -ldw $QUERY $DATA/index.mmi
echo "[PERF] Running minimap2"
start_date=`date +%s%N`
minimap2 -t 32 -x $MM2_PRESET -o $DATA/mapping.paf $DATA/index.mmi $QUERY
end_date=`date +%s%N`
echo `expr $end_date - $start_date` > performances_mm2.txt
pkill vmtouch

rm -f $DATA/index.dti
rm -f $DATA/index.mmi
rm -f $DATA/mapping.paf
