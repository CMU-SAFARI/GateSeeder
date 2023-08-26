#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <ont|hifi|illumina>"
    exit 1
fi

if [ $1 = 'ont' ]; then
	query=$DATA/HG002_ONT-UL_GIAB_20200204_1000filtered_2Mreads.fasta
	w=10
	k=15
	max_occ=(10 50 100)
	batch_size=(67108864 33554432 16777216)
	vt_distance=(950 750 700)
	extra_param=""
elif [ $1 = 'hifi' ]; then
	query=$DATA/m64011_190830_220126.fasta
	w=19
	k=19
	max_occ=(1 2 5)
	batch_size=(134217728 134217728 134217728)
	vt_distance=(5000 4000 4000)
	extra_param=""
elif [ $1 = 'illumina' ]; then
	query=$DATA/D1_S1_L001_R1_001-017_header.fasta
	w=11
	k=21
	max_occ=(50 150 450)
	batch_size=(33554432  33554432 33554432)
	vt_distance=(10 10 10)
	extra_param="-e -c 0 -s"
else
    echo "Usage: $0 <ont|hifi|illumina>"
	exit 1
fi

target=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
paf=$DATA/mapping.paf
res_dir=profile_$1
xclbin=../device/demeter_$1.xclbin

mkdir -p $res_dir
rm -f $res_dir/*

for i in "${!max_occ[@]}";
do
	echo "[PROF] Generating the index with max_occ: ${max_occ[i]}"
	../GateSeeder_index -t 32 -w $w -k $k -f ${max_occ[i]} $target $DATA/index.sfi
	echo "[PROF] Compiling GateSeeder:"
	make -C .. clean
	make -C .. ADDFLAGS=-DPROFILE
	echo "[PROF] Running GateSeeder on fpga:"
	../GateSeeder -t 32 -b ${batch_size[i]} -d ${vt_distance[i]} $extra_param $xclbin $DATA/index.sfi $query -o $paf > $res_dir/fpga_$i.dat
	echo "[PROF] Compiling GateSeeder:"
	make -C .. clean
	make -C .. ADDFLAGS='-DPROFILE -DCPU_EX'
	echo "[PROF] Running GateSeeder on cpu:"
	../GateSeeder -t 32 -b ${batch_size[i]} -d ${vt_distance[i]} $extra_param $xclbin $DATA/index.sfi $query -o $paf > $res_dir/cpu_$i.dat
done

make -C .. clean
rm -f $DATA/index.sfi $paf
