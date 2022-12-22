open_project -reset demeter_vitis_hls
add_files "src/kernel.cpp src/extraction.cpp src/querying.cpp"
add_files -tb -cflags "-Iutil_src -Isrc" "tb_src/top.cpp"
add_files -tb "../util_src/parsing.c"
set_top demeter_kernel
open_solution -flow_target vitis solution1
set_part {xcu280-fsvh2892-2L-e}
create_clock -period "350MHz"
