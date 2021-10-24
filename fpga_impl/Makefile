HLS_ROOT		?= /opt/xilinx/Vitis_HLS/2021.1

SRC_DIR		:= src
CXX		:= g++

DEPFLAGS	= -MD -MP
CPPFLAGS	= -I $(HLS_ROOT)/include
CXXFLAGS	= -O3 -g $(DEPFLAGS)
VPATH		= $(SRC_DIR)
SIM		= sim.x

SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=%.o)

vitis:
	vitis_hls -f scripts/run_hls.tcl

sim: $(SIM)
	./$(SIM) ../res/ERR240727_1.fastq ../res/c_w12_k18_f500_b26.bin

$(SIM): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4}" src/*.cpp src/*.hpp

clean:
	$(RM) -r work *.o *.d $(SIM)

.PHONY: vitis format clean sim
