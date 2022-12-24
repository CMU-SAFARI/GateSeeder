source scripts/vitis_hls.tcl
add_files -tb "/mnt/scratch/jeudine/chr2-3_w10_k15_f200.dti ../test/dummy1.fastq ../util/demeter_util.a"
#csim_design -O -ldflags {-z stack-size=2147483648} -argv "index.ali dummy1.fastq 1"
csynth_design
cosim_design -O -ldflags {-z stack-size=10485760 demeter_util.a} -argv "chr2-3_w10_k15_f200.dti dummy1.fastq"
exit
