#!/bin/bash

RES=performances_demeter.txt
TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta

if [ $1 = 'ont' ]; then
	XCLBIN=../device/demeter_ont.xclbin
	QUERY=$DATA/HG002_ONT-UL_GIAB_20200204_1000filtered_2Mreads.fastq
	RANGE_MAX_OCC=$(eval echo "{10..100..10}")
	W=10
	K=15
	MM2_PRESET='map-ont'
elif [ $1 = 'hifi' ]; then
	XCLBIN=../device/demeter_hifi.xclbin
	QUERY=$DATA/m64011_190830_220126.fastq
	W=19
	K=19
	RANGE_MAX_OCC=$(eval echo "{1..5..1}")
	MM2_PRESET='map-hifi'
else
	exit 1
fi


rm -f $RES

for max_occ in $RANGE_MAX_OCC
do
	echo "[ACC] Generating the index with max_occ: $max_occ"
	../demeter_index -t 32 -w $W -k $K -f $max_occ $TARGET $DATA/index.dti
	echo "[ACC] Evicting the index and the reads from the page cache"
	./page_cache_evict $DATA/index.dti $QUERY
	echo "[ACC] Running demeter"
	start_date=`date +%s%N`
	../demeter -t 32 $XCLBIN $DATA/index.dti $QUERY -o $DATA/mapping.paf
	end_date=`date +%s%N`
	echo `expr $end_date - $start_date` >> $RES
done

echo "[ACC] Generating the index for minimap2"
minimap2 -t 32 -x $MM2_PRESET -d $DATA/index.mmi $TARGET
echo "[ACC] Evicting the index and the reads from the page cache"
./page_cache_evict $DATA/index.mmi $QUERY
echo "[ACC] Running minimap2"
start_date=`date +%s%N`
minimap2 -t 32 -x $MM2_PRESET -o $DATA/mapping.paf $DATA/index.mmi $QUERY
end_date=`date +%s%N`
echo `expr $end_date - $start_date` > performances_mm2.txt

rm -f $DATA/index.dti
rm -f $DATA/index.mmi
rm -f $DATA/mapping.paf
