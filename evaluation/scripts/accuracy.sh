#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <ont|hifi|illumina>"
    exit 1
fi

query=$DATA/$1_sim.fasta

if [ $1 = 'ont' ]; then
	w=10
	k=15
	range_max_occ="10 50 100"
	range_vt_distance=$(eval echo "{500..1000..50}")
	extra_param=""
	minimap2_preset='map-ont'
	nb_reads=$(expr $(wc -l < $query) / 2)
elif [ $1 = 'hifi' ]; then
	w=19
	k=19
	range_max_occ="1 2 5"
	range_vt_distance=$(eval echo "{1000..5000..500}")
	extra_param=""
	minimap2_preset='map-hifi'
	nb_reads=$(expr $(wc -l < $query) / 2)
elif [ $1 = 'illumina' ]; then
	w=11
	k=21
	range_max_occ="50 150 450"
	range_vt_distance="10 20 30 50"
	extra_param="-e -c 0 -s"
	minimap2_preset='sr'
	nb_reads=$(expr $(wc -l < $query) / 4)
else
    echo "Usage: $0 <ont|hifi|illumina>"
	exit 1
fi

target=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
paf=$DATA/mapping.paf

xclbin=../device/demeter_$1.xclbin
res_dir=accuracy_$1
mkdir -p $res_dir
rm -f $res_dir/*

echo -e "$1\t$nb_reads" > $res_dir/info.dat

for max_occ in $range_max_occ
do
	echo "[ACC] Generating the index with max_occ: $max_occ"
	../seedfarm_index -t 32 -w $w -k $k -f $max_occ $target $DATA/index.sfi
	for vt_distance in $range_vt_distance
	do
		echo "[ACC] Running seedfarm with vt_distance: $vt_distance"
		../seedfarm -t 32 -d $vt_distance $extra_param $xclbin $DATA/index.sfi $query -o $paf
		echo "[ACC] Evaluating the results"
		echo -e "max_occ: $max_occ, vt_dst: $vt_distance" > $res_dir/seedfarm_${max_occ}_${vt_distance}_$1.dat
		paftools.js mapeval $paf >> $res_dir/seedfarm_${max_occ}_${vt_distance}_$1.dat
	done
done

echo "[ACC] Running minimap2"
minimap2 -t 32 -x $minimap2_preset -o $paf $target $query
echo "[ACC] Evaluating the results"
echo -e "minimap2" > $res_dir/minimap2_$1.dat
paftools.js mapeval $paf >> $res_dir/minimap2_$1.dat

rm -f $DATA/index.sfi
rm -f $paf
