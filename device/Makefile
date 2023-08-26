CXX := g++
SRC_DIR := src
UTIL_DIR := ../util
CONFIG_DIR := config

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=%.o)
UTIL = $(UTIL_DIR)/GateSeeder_util.a

VPATH = $(SRC_DIR)

CPPFLAGS = -I$(XILINX_VITIS_HLS)/include -I$(UTIL_DIR)/include
DEPFLAGS = -MD -MP
CXXFLAGS = -std=c++17 -O3 -g -march=native $(DEPFLAGS) -Wall
LDLIBS = -lstdc++ $(UTIL)

PARAMS = $(wildcard $(CONFIG_DIR)/*.param)
CONFIG = $(CONFIG_DIR)/GateSeeder.cfg
XCLBINS = $(PARAMS:$(CONFIG_DIR)/%.param=GateSeeder_%.xclbin)
SIM = GateSeeder_sim

all: $(XCLBINS)

GateSeeder_%.xclbin: $(CONFIG_DIR)/%.param auto_gen $(CONFIG)
	./auto_gen $< > src/auto_gen.hpp
	vitis_hls scripts/impl.tcl
	v++  --kernel_frequency 317 -t hw --link GateSeeder.xo -o $@ --config $(CONFIG)

auto_gen: src/auto_gen.rs
	rustc $<

sim: GateSeeder_util $(SIM)

$(SIM): $(OBJS) $(UTIL)
	$(LINK.o) $(OBJS) $(LOADLIBES) $(LDLIBS) -o $@

#$(OBJS): CPPFLAGS+=-DDEBUG_SEED_EXTRACTION
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_MAP
#$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_KEY

GateSeeder_util:
	$(MAKE) -C $(UTIL_DIR)

clean:
	$(RM) *.o *.d $(SIM) *.log *.xo


cosim: GateSeeder_util
	vitis_hls scripts/cosim.tcl


.PHONY: clean sim cosim GateSeeder_util all
-include $(wildcard *.d)
