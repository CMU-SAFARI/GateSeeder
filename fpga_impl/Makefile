HLS_ROOT		?= /opt/xilinx/Vitis_HLS/2021.1

SRC_DIR		:= src
CXX		:= g++

DEPFLAGS	= -MD -MP
CPPFLAGS	= -I $(HLS_ROOT)/include
CXXFLAGS	= -O3 -g $(DEPFLAGS) -Wunused
VPATH		= $(SRC_DIR)
SIM		= sim.x

SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=%.o)

FASTQ		:= ../res/illumina_1000.fastq
BIN 		:= ../res/chr1_k18_w12_f700_b26.bin
DAT			:= ../res/illumina_1000_chr1_k18_w12_f700_b26_m3_l150.dat

vitis:
	vitis_hls -f scripts/run_hls.tcl

sim: $(SIM)
	./$(SIM) $(FASTQ) $(BIN) $(DAT)

$(SIM): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

format:
	clang-format -i src/*.cpp src/*.hpp

clean:
	$(RM) -r work *.o *.d $(SIM)

.PHONY: vitis format clean sim
-include $(wildcard *.d)
