CXX	:= g++

SRC_DIR		:= src
SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=%.o)

SRCS_HPP = $(wildcard $(SRC_DIR)/*.hpp)

TB_SRC_DIR	:= tb_src
TB_SRCS		= $(wildcard $(TB_SRC_DIR)/*.cpp)
TB_OBJS		= $(TB_SRCS:$(TB_SRC_DIR)/%.cpp=%.o)

UTIL_SRC_DIR 	:= util_src
UTIL_SRCS		= $(wildcard $(UTIL_SRC_DIR)/*.c)
UTIL_OBJS		= $(UTIL_SRCS:$(UTIL_SRC_DIR)/%.c=%.o)

HOST_SRC_DIR	:= host_src
HOST_SRCS		= $(wildcard $(HOST_SRC_DIR)/*.cpp)
HOST_OBJS		= $(HOST_SRCS:$(HOST_SRC_DIR)/%.cpp=%.o)

VPATH	= $(SRC_DIR) $(TB_SRC_DIR) $(UTIL_SRC_DIR) $(HOST_SRC_DIR)

DEPFLAGS	= -MD -MP
CXXFLAGS	= -O3 -g $(DEPFLAGS) -Wall -Wextra

SIM			= demeter_sim
HOST		= demeter

XOBJ		= demeter.xo
XCLBIN		= demeter.xclbin
CONFIG		= config/demeter.cfg

$(SIM): $(OBJS) $(TB_OBJS) $(UTIL_OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
$(SIM): LDLIBS+=-lstdc++
$(TB_OBJS): CPPFLAGS+=-I$(UTIL_SRC_DIR) -I$(SRC_DIR) -I$(XILINX_VITIS_HLS)/include
$(OBJS): CPPFLAGS+=-I$(XILINX_VITIS_HLS)/include

#$(OBJS): CPPFLAGS+=-DDEBUG_SEED_EXTRACTION
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_MAP
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_KEY

clean:
	$(RM) *.o *.d $(SIM) *.log *.xo $(HOST)

sim: $(SIM)
	./$< /mnt/scratch/jeudine/chr2-3.dti ../test/dummy1.fastq

cosim:
	vitis_hls scripts/cosim.tcl

$(XOBJ): $(SRCS) $(SRCS_HPP)
	vitis_hls scripts/impl.tcl

$(XCLBIN): $(XOBJ) $(CONFIG)
	v++ -t hw --link $(XOBJ) -o $@ --config $(CONFIG)

$(HOST): $(HOST_OBJS) $(UTIL_OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(HOST): LDFLAGS+=-L$(XILINX_XRT)/lib
$(HOST): LDLIBS+=-lxrt_coreutil -pthread -lstdc++
$(HOST_OBJS): CPPFLAGS+=-I$(XILINX_XRT)/include -I$(UTIL_SRC_DIR)
$(HOST_OBJS): CXXFLAGS+=-std=c++17

.PHONY: clean sim cosim
-include $(wildcard *.d)
