CXX	:= g++

XILINX_ROOT	?= /opt/Xilinx
HLS_ROOT	= $(XILINX_ROOT)/Vitis_HLS/2022.1
VITIS_ROOT	= $(XILINX_ROOT)/Vitis/2022.1

SRC_DIR		:= src
SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=%.o)

SRCS_HPP = $(wildcard $(SRC_DIR)/*.hpp)

TB_SRC_DIR	:= tb_src
TB_SRCS		= $(wildcard $(TB_SRC_DIR)/*.cpp)
TB_OBJS		= $(TB_SRCS:$(TB_SRC_DIR)/%.cpp=%.o)

UTIL_SRC_DIR := util_src
UTIL_SRCS		= $(wildcard $(UTIL_SRC_DIR)/*.c)
UTIL_OBJS		= $(UTIL_SRCS:$(UTIL_SRC_DIR)/%.c=%.o)

VPATH	= $(SRC_DIR) $(TB_SRC_DIR) $(UTIL_SRC_DIR)

DEPFLAGS	= -MD -MP
CPPFLAGS	= -I$(HLS_ROOT)/include
CXXFLAGS	= -O3 -g $(DEPFLAGS) -Wall -Wextra

TB			= tb.x


XOBJ		= demeter.xo
XCLBIN		= demeter.xclbin
CONFIG		= config/demeter.cfg


$(TB): $(OBJS) $(TB_OBJS) $(UTIL_OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
$(TB): LDLIBS+=-lstdc++
$(TB_OBJS): CPPFLAGS+=-I$(UTIL_SRC_DIR) -I$(SRC_DIR)

#$(OBJS): CPPFLAGS+=-DDEBUG_SEED_EXTRACTION
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_MAP
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_KEY

clean:
	$(RM) *.o *.d $(TB)

sim: $(TB)
	./$< ../indexing/index.ali ../test/dummy1.fastq 4

cosim:
	LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$(LIBRARY_PATH) $(HLS_ROOT)/bin/vitis_hls scripts/cosim.tcl

$(XOBJ): $(SRCS) $(SRCS_HPP)
	LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$(LIBRARY_PATH) $(HLS_ROOT)/bin/vitis_hls scripts/impl.tcl

$(XCLBIN): $(XOBJ) $(CONFIG)
	LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$(LIBRARY_PATH) $(VITIS_ROOT)/bin/v++ -t hw --link $(XOBJ) -o $@ --config $(CONFIG)



.PHONY: clean sim cosim
-include $(wildcard *.d)
