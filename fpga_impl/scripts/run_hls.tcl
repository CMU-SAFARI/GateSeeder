# Create a project
open_project -reset work

# Add design files
add_files -cflags "-O3 -Wall" "src/seeding.cpp src/extraction.cpp"
# Add test bench & files
add_file -cflags "-O3" -tb "src/tb_top.cpp src/tb_driver.cpp"

# Set the top-level function
set_top seeding

# ########################################################
# Create a solution
open_solution -flow_target vitis -reset solution1
# Define technology and clock rate
set_part {xcvu37p-fsvh2892-3-e}
create_clock -period 8

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
        cosim_design -O -argv "../res/ERR240727_1.fastq ../res/c_w12_k18_f500_b26.bin ../res/ch1_w12_k_18_f500_b26_l150.data"
}

exit
