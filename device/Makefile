CXX := g++
SRC_DIR := src
UTIL_DIR := ../util

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=%.o)
UTIL = $(UTIL_DIR)/demeter_util.a

VPATH = $(SRC_DIR)

CPPFLAGS = -I$(XILINX_VITIS_HLS)/include -I$(UTIL_DIR)/include
DEPFLAGS = -MD -MP
CXXFLAGS = -std=c++17 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra
LDLIBS = -lstdc++ $(UTIL)

SIM = demeter_sim

XOBJ = demeter.xo
XCLBIN = demeter.xclbin
CONFIG = config/demeter.cfg

$(SIM): $(OBJS) demeter_util
	$(LINK.o) $(OBJS) $(LOADLIBES) $(LDLIBS) -o $@

#$(OBJS): CPPFLAGS+=-DDEBUG_SEED_EXTRACTION
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_MAP
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_KEY

demeter_util:
	$(MAKE) -C $(UTIL_DIR)

clean:
	$(RM) *.o *.d $(SIM) *.log *.xo

sim: $(SIM)
	./$< /mnt/scratch/jeudine/chr2-3.dti ../test/dummy.fastq

cosim: demeter_util
	vitis_hls scripts/cosim.tcl

$(XOBJ): $(SRC_DIR)/*
	vitis_hls scripts/impl.tcl

$(XCLBIN): $(XOBJ) $(CONFIG)
	v++ -t hw --link $(XOBJ) -o $@ --config $(CONFIG)

.PHONY: clean sim cosim demeter_util
-include $(wildcard *.d)
