CXX	:= g++
HLS_ROOT	?= /opt/xilinx/Vitis_HLS/2022.1

DEPFLAGS	= -MD -MP
CPPFLAGS	= -I$(HLS_ROOT)/include
CXXFLAGS	= -O3 -g $(DEPFLAGS) -Wall

SRC_DIR		:= src
SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=%.o)

VPATH	= $(SRC_DIR)

SIM			= sim.x

$(SIM): $(OBJS)


clean:
	$(RM) *.o
