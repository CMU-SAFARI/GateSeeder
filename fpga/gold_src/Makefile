CXX	:= g++

HLS_ROOT	?= /opt/xilinx/Vitis_HLS/2022.1


SRCS		= $(wildcard *.c)
OBJS		= $(SRCS:%.c=%.o)

DEPFLAGS	= -MD -MP
CXXFLAGS	= -O3 -g $(DEPFLAGS) -Wall -Wextra

GOLD			= gold.x

$(GOLD): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) *.o *.d $(GOLD)

.PHONY: clean
-include $(wildcard *.d)
