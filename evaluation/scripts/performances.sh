#!/bin/bash

RES=accuracy_demeter.txt
TARGET=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
QUERY=$DATA/
XCLBIN=../device/demeter.xclbin

rm -f $RES

for max_occ in {50..700..50}
do
	echo "[ACC] Generating the index with max_occ: $max_occ"
	../demeter_index -t 32 -f $max_occ $TARGET index.dti
	echo "[ACC] Evicting the index and the reads from the page cache"
	./page_cache_evict index.dti $QUERY
	echo "[ACC] Running demeter"
	../demeter -t 32 $XCLBIN index.dti $QUERY -o mapping.paf 2>> $RES
done
rm -f index.dti

echo "[ACC] Generating the index for minimap2"
minimap2 -t 32 -x map-ont -o mapping.paf $TARGET $QUERY
