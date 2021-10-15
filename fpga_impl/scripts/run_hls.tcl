# Create a project
open_project -reset seeding

# Add design files
add_files src/seeding.cpp
# Add test bench & files
#add_files -tb

# Set the top-level function
set_top seeding

# ########################################################
# Create a solution
open_solution -flow_target vitis -reset solution1
# Define technology and clock rate
set_part {xcvu9p-flga2104-2-i}
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
}

exit