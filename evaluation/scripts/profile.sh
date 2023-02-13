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
elif [ $1 = 'hifi' ]; then
	query=$DATA/m64011_190830_220126.fasta
	w=19
	k=19
	max_occ=(1 2 5)
	batch_size=(134217728 134217728 134217728)
elif [ $1 = 'illumina' ]; then
	query=$DATA/D1_S1_L001_R1_001-017_header.fasta
	w=11
	k=21
	max_occ=(50 150 450)
	batch_size=(33554432  33554432 33554432)
else
    echo "Usage: $0 <ont|hifi|illumina>"
	exit 1
fi

target=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
res_dir=profile_$1
xclbin=../device/demeter_$1.xclbin

mkdir -p $res_dir
rm -f $res_dir/*

for i in "${!max_occ[@]}";
do
	echo "[PERF] Generating the index with max_occ: ${max_occ[i]}"
	../seedfarm_index -t 32 -w $w -k $k -f ${max_occ[i]} $target $DATA/index.sfi
	echo "[PERF] Running seedfarm_profile for FPGA: 1CU"
	../seedfarm_profile -t 1 -b ${batch_size[i]} -f $xclbin $DATA/index.sfi $query > $res_dir/fpga_1_$i.dat
	echo "[PERF] Running seedfarm_profile for FPGA: 8CUs"
	../seedfarm_profile -t 8 -b ${batch_size[i]} -f $xclbin $DATA/index.sfi $query > $res_dir/fpga_8_$i.dat
	echo "[PERF] Running seedfarm_profile for CPU: 1thread"
	../seedfarm_profile -t 1 -b ${batch_size[i]} $DATA/index.sfi $query > $res_dir/cpu_1_$i.dat
	echo "[PERF] Running seedfarm_profile for CPU: 32threads"
	../seedfarm_profile -t 32 -b ${batch_size[i]} $xclbin $DATA/index.sfi $query > $res_dir/cpu_32_$i.dat
done

rm -f $DATA/index.sfi
