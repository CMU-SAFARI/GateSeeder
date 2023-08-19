CC := gcc

SRC_DIR := src

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=%.o)

VPATH = $(SRC_DIR)

CPPFLAGS += -Iinclude
DEPFLAGS = -MD -MP
CFLAGS = -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror -std=gnu11
ARFLAGS = -rsc

LIB = seedfarm_util.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $<

clean:
	$(RM) $(LIB) *.o *.d

.PHONY: all clean
-include $(wildcard *.d)
