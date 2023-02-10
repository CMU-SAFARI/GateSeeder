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
	minimap2_preset='map-ont'
elif [ $1 = 'hifi' ]; then
	query=$DATA/m64011_190830_220126.fasta
	w=19
	k=19
	max_occ=(1 2 5)
	batch_size=(134217728 134217728 134217728)
	vt_distance=(5000 4000 4000)
	extra_param=""
	minimap2_preset='map-hifi'
elif [ $1 = 'illumina' ]; then
	query=$DATA/D1_S1_L001_R1_001-017_header.fasta
	w=11
	k=21
	max_occ=(50 150 450)
	batch_size=(33554432  33554432 33554432)
	vt_distance=(10 10 10)
	extra_param="-e -c 0 -s"
	minimap2_preset='sr'
else
    echo "Usage: $0 <ont|hifi|illumina>"
	exit 1
fi

target=$DATA/GCA_000001405.15_GRCh38_no_alt_analysis_set.fasta
paf=$DATA/mapping.paf

xclbin=../device/demeter_$1.xclbin
res=performance_$1.dat

rm -f $res

echo "[PERF] Loading the xclbin"
xbutil program -d 0000:c4:00.1 -u $xclbin

for i in "${!max_occ[@]}";
do
	echo "[PERF] Generating the index with max_occ: ${max_occ[i]}"
	../seedfarm_index -t 32 -w $w -k $k -f ${max_occ[i]} $target $DATA/index.sfi
	echo "[PERF] Locking the reads and the index into RAM"
	vmtouch -ldw $query $DATA/index.sfi
	echo "[PERF] Running seedfarm"
	echo -e  "../seedfarm -t 32 -b ${batch_size[i]} -d ${vt_distance[i]} $extra_param $xclbin $DATA/index.sfi $query -o $paf"
	start_date=`date +%s%N`
	../seedfarm -t 32 -b ${batch_size[i]} -d ${vt_distance[i]} $extra_param $xclbin $DATA/index.sfi $query -o $paf
	end_date=`date +%s%N`
	echo `expr $end_date - $start_date` >> $res
	pkill vmtouch
	rm -f $paf
done

echo "[PERF] Generating the index for minimap2"
minimap2 -t 32 -x $minimap2_preset -d $DATA/index.mmi $target
echo "[PERF] Locking the reads and the index into RAM"
vmtouch -ldw $query $DATA/index.mmi
echo "[PERF] Running minimap2"
start_date=`date +%s%N`
minimap2 -t 32 -x $minimap2_preset -o $paf $DATA/index.mmi $query
end_date=`date +%s%N`
echo `expr $end_date - $start_date` >> $res
pkill vmtouch
rm -f $paf

rm -f $DATA/index.sfi
rm -f $DATA/index.mmi
