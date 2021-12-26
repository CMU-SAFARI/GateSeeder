#!/bin/bash
rm -f sensitivity.dat
for w in {10..50}
do
	echo -e "SENSITIVITY:\t\tW: $w"
	../dna_indexing/indexdna -w$w -k19 -b26 -f2000 ../res/GCF_000001405.39_GRCh38.p13_genomic.fna /tmp/idx
	cd ../cpu_impl
	make clean
	CFLAGS="-DW=$w" make
	start_time=`date +%s`
	./map-pacbio -i/tmp/idx.bin ../res/pacbio_10000.bin -o/tmp/locs
	end_time=`date +%s`
	exec_time_0=`expr $end_time - $start_time`
	echo -e "SENSITIVITY\t\tEXEC_TIME: $exec_time_0"
	cd ../sensitivity
	threshold=`cargo run --release -- ../res/gold_pacbio_10000.paf /tmp/locs.dat`
	echo -e "SENSITIVITY\t\tTHRESHOLD: $threshold"
	cd ../cpu_impl
	make clean
	CFLAGS="-DW=$w -DADJACENCY_FILTERING_THRESHOLD=$threshold -DADJACENCY_FILTERING" make
	start_time=`date +%s`
	./map-pacbio -i/tmp/idx.bin ../res/pacbio_10000.bin -o/tmp/locs
	end_time=`date +%s`
	exec_time_1=`expr $end_time - $start_time`
	echo -e "SENSITIVITY\t\tEXEC_TIME: $exec_time_1"
	cd ../sensitivity
	stats=`cargo run --release -- -s ../res/gold_pacbio_10000.paf /tmp/locs.dat`
	echo -e "SENSITIVITY\t\tSTATS: $stats"
	echo -e "$w\t$threshold\t$stats\t$exec_time_0\t$exec_time_1" >> sensitivity.dat
done

