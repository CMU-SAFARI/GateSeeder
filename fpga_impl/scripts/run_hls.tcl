set FASTQ illumina_100.fastq
set DAT illumina_100_chr1_k18_w12_f700_b26_m3_l150.dat
set BIN chr1_k18_w12_f700_b26.bin

# Create a project
open_project -reset work

# Add design files
add_files -cflags "-O3 -Wno-unknown-pragmas -Wno-unused-label" "src/seeding.cpp src/extraction.cpp"
# Add test bench & files
add_files -cflags "-O3" -tb "src/tb_top.cpp src/tb_driver.cpp"
add_files -tb "../res/$FASTQ"
add_files -tb "../res/$DAT"
add_files -tb "../res/$BIN"

# Set the top-level function
set_top seeding

# ########################################################
# Create a solution
open_solution -flow_target vitis -reset solution1

# Define technology and clock rate
set_part {xcvu37p-fsvh2892-3-e}
create_clock -period 6

set hls_exec 0
#csim_design
# Set any optimization directives
# End of directives

if {$hls_exec == 1} {
	# Run Synthesis and Exit
	csynth_design

} elseif {$hls_exec == 2} {
	# Run Synthesis, RTL Simulation and Exit
	csynth_design

	cosim_design
} elseif {$hls_exec == 3} {
	# Run Synthesis, RTL Simulation, RTL implementation and Exit
	csynth_design

	cosim_design
	export_design
} else {
	# Default is to exit after setup
	csynth_design
	#cosim_design -O -argv "$FASTQ $BIN $DAT"
	#cosim_design -trace_level all
}

exit
