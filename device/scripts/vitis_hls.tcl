open_project -reset demeter_vitis_hls
add_files "src/kernel.cpp src/extraction.cpp src/querying.cpp"
add_files -tb -cflags "-I../util/include" "src/tb.cpp"
set_top demeter_kernel
open_solution -flow_target vitis solution1
#set_part {xcu280-fsvh2892-2L-e}
set_part {xcu55c-fsvh2892-2L-e}
create_clock -period "350MHz"
