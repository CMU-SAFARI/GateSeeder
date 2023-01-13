#!/bin/bash

RES=performances_demeter.txt
TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
QUERY=$DATA/HG002_ONT-UL_GIAB_20200204_1000filtered_2Mreads.fastq
XCLBIN=../device/demeter.xclbin

#rm -f $RES
#
#for max_occ in {50..700..50}
#do
#	echo "[ACC] Generating the index with max_occ: $max_occ"
#	../demeter_index -t 32 -f $max_occ $TARGET $DATA/index.dti
#	echo "[ACC] Evicting the index and the reads from the page cache"
#	./page_cache_evict $DATA/index.dti $QUERY
#	echo "[ACC] Running demeter"
#	../demeter -t 32 $XCLBIN $DATA/index.dti $QUERY -o $DATA/mapping.paf 2>> $RES
#done

echo "[ACC] Generating the index for minimap2"
minimap2 -t 32 -x map-ont -d $DATA/index.mmi $TARGET
echo "[ACC] Evicting the index and the reads from the page cache"
./page_cache_evict $DATA/index.mmi $QUERY
minimap2 -t 32 -x map-ont -o mapping.paf $DATA/index.mmi $QUERY 2>> performances_minimap2.txt

rm -f $DATA/index.dti
rm -f $DATA/index.mmi
rm -f $DATA/mapping.paf
