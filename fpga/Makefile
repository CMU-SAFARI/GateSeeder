CXX	:= g++

HLS_ROOT	?= /opt/xilinx/Vitis_HLS/2022.1


SRC_DIR		:= src
SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=%.o)

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

$(TB): $(OBJS) $(TB_OBJS) $(UTIL_OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
$(TB): LDLIBS+=-lstdc++
$(TB_OBJS): CPPFLAGS+=-I$(UTIL_SRC_DIR) -I$(SRC_DIR)

#$(OBJS): CPPFLAGS+=-DDEBUG_SEED_EXTRACTION
$(OBJS): CPPFLAGS+=-DDEBUG_QUERY_INDEX_MAP

clean:
	$(RM) *.o *.d $(TB)

.PHONY: clean
-include $(wildcard *.d)
