HLS_ROOT		?= /opt/xilinx/Vitis_HLS/2020.2

SRC_HW_DIR		:= src/hw
SRC_TB_DIR		:= src/tb

CORE			:= core
COMPOSE			:= compose
PLATFORM		:= xupvvh
EFFORT			:= fastest
TAPFLAGS		= -v --KernelDir tapasco --parallel

CXX				:= g++
DEPFLAGS		= -MD -MP
CPPFLAGS		= -I $(HLS_ROOT)/include
CXXFLAGS		= -O3 -g $(DEPFLAGS) -Wall -Werror -Wno-unknown-pragmas -Wno-unused-label
VPATH			= $(SRC_HW_DIR) $(SRC_TB_DIR)
SIM				= sim.x

SRCS_HW			= $(wildcard $(SRC_HW_DIR)/*.cpp)
SRCS_TB			= $(wildcard $(SRC_TB_DIR)/*.cpp)
OBJS_HW			= $(SRCS_HW:$(SRC_HW_DIR)/%.cpp=%.o)
OBJS_TB			= $(SRCS_TB:$(SRC_TB_DIR)/%.cpp=%.o)

XO				= seeding.xo

TB_ID			:= 1000
FASTQ			:= ../res/illumina_$(TB_ID).fastq
BIN 			:= ../res/chr1_k18_w12_f700_b26.bin
DAT				:= ../res/illumina_$(TB_ID)_chr1_k18_w12_f700_b26_m3_l150.dat

$(XO): $(SRCS_HW)
	vitis_hls -f scripts/impl.tcl

cosim: $(XO)
	vitis_hls -f scripts/cosim.tcl

sim: $(SIM)
	./$(SIM) $(FASTQ) $(BIN) $(DAT)

$(SIM): $(OBJS_HW) $(OBJS_TB)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

format:
	clang-format -i $(SRC_HW_DIR)/*.cpp $(SRC_HW_DIR)/*.hpp $(SRC_TB_DIR)/*.cpp $(SRC_TB_DIR)/*.hpp

bitstream: $(COMPOSE)

$(CORE): $(SRCS_HW)
	$(RM) -r $(CORE)
	tapasco $(TAPFLAGS) --coreDir $(CORE) hls seeding -p $(PLATFORM)

$(COMPOSE): $(CORE)
	$(RM) -r $(COMPOSE)
	tapasco --configFile tapasco/config.json

	#tapasco $(TAPFLAGS) --coreDir $(CORE) --compositionDir $(COMPOSE) compose [seeding x 1] @ 150 Mhz -p $(PLATFORM) --effortLevel $(EFFORT) --skipSynthesis --deleteProjects false

clean:
	$(RM) -r vitis_hls *.o *.d $(XO) $(SIM) $(CORE) $(COMPOSE)

.PHONY: vitis format clean sim cosim bitstream
-include $(wildcard *.d)
