CC := gcc
CXX := g++

SRC_DIR := src
UTIL_DIR := ../util

CXX_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
CXX_OBJS = $(CXX_SRCS:$(SRC_DIR)/%.cpp=%.o)

C_SRCS = $(wildcard $(SRC_DIR)/*.c)
C_OBJS = $(C_SRCS:$(SRC_DIR)/%.c=%.o)

UTIL = $(UTIL_DIR)/GateSeeder_util.a

VPATH = $(SRC_DIR)

CPPFLAGS += -I$(XILINX_XRT)/include -I$(UTIL_DIR)/include #-DPROFILE
DEPFLAGS = -MD -MP
CXXFLAGS = -std=c++17 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror
CFLAGS = -std=gnu11 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror
LDFLAGS += -L$(XILINX_XRT)/lib
LDLIBS += -lxrt_coreutil -pthread -lstdc++ -lm $(UTIL)

EXE := GateSeeder_profile

all: GateSeeder_util $(EXE)

$(EXE): $(C_OBJS) $(CXX_OBJS) $(UTIL)
	$(LINK.o) $(C_OBJS) $(CXX_OBJS) $(LOADLIBES) $(LDLIBS) -o $@

GateSeeder_util:
	$(MAKE) -C $(UTIL_DIR)

clean:
	$(RM) *.o *.d $(EXE)

.PHONY: all clean GateSeeder_util
-include $(wildcard *.d)
