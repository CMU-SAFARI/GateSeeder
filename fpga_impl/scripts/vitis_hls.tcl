open_project vitis_hls
add_files -cflags "-O3 -Wno-unknown-pragmas -Wno-unused-label" "src/hw/seeding.cpp src/hw/extraction.cpp"
set_top seeding
open_solution -flow_target vitis solution1
set_part {xcvu37p-fsvh2892-3-e}
create_clock -period 6
