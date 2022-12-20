CC := gcc
CXX := g++

SRC_DIR := src
CXX_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
CXX_OBJS = $(CXX_SRCS:$(SRC_DIR)/%.cpp=%.o)

C_SRCS = $(wildcard $(SRC_DIR)/*.c)
C_OBJS = $(C_SRCS:$(SRC_DIR)/%.c=%.o)

UTIL_SRC_DIR := ../util_src
UTIL_SRCS = $(wildcard $(UTIL_SRC_DIR)/*.c)
UTIL_OBJS = $(UTIL_SRCS:$(UTIL_SRC_DIR)/%.c=%.o)

VPATH = $(SRC_DIR) $(UTIL_SRC_DIR)

CPPFLAGS += -I$(XILINX_XRT)/include -I$(UTIL_SRC_DIR) -DPROFILE
DEPFLAGS = -MD -MP
CXXFLAGS = -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror -std=c++17
CFLAGS = -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror -std=gnu11
LDFLAGS += -L$(XILINX_XRT)/lib
LDLIBS += -lxrt_coreutil -pthread -lstdc++

EXE := demeter

$(EXE): $(C_OBJS) $(CXX_OBJS) $(UTIL_OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) *.o *.d $(EXE)

.PHONY: clean
-include $(wildcard *.d)
