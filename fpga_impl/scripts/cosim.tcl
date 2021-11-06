source scripts/vitis_hls.tcl
set FASTQ illumina_5.fastq
set DAT illumina_5_chr1_k18_w12_f700_b26_m3_l150.dat
set BIN chr1_k18_w12_f700_b26.bin
add_files -cflags "-O3" -tb "src/tb/tb_top.cpp src/tb/tb_driver.cpp"
add_files -tb "../res/$FASTQ"
add_files -tb "../res/$DAT"
add_files -tb "../res/$BIN"
cosim_design -O -argv "$FASTQ $BIN $DAT"
exit
