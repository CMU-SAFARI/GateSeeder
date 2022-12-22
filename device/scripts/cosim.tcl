source scripts/vitis_hls.tcl
add_files -tb "../indexing/index.ali ../test/dummy1.fastq"
#csim_design -O -ldflags {-z stack-size=2147483648} -argv "index.ali dummy1.fastq 1"
csynth_design
cosim_design -O -ldflags {-z stack-size=10485760} -argv "index.ali dummy1.fastq 1"
exit
