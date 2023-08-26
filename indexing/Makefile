CC := gcc

SRC_DIR := src
UTIL_DIR := ../util

CPPFLAGS += -I$(UTIL_DIR)/include
DEPFLAGS = -MD -MP
CFLAGS = -std=gnu11 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror
LDLIBS = -pthread

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=%.o)

VPATH = src

EXE := GateSeeder_index

all: $(EXE)

$(EXE): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) *.o *.d $(EXE)

.PHONY: all clean
-include $(wildcard *.d)
