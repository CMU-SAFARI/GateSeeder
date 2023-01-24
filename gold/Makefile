CC := gcc

SRC_DIR := src
UTIL_DIR := ../util

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=%.o)
UTIL = $(UTIL_DIR)/demeter_util.a

CPPFLAGS = -I$(UTIL_DIR)/include
DEPFLAGS = -MD -MP
CFLAGS = -std=gnu11 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror
LDLIBS = $(UTIL)
VPATH = $(SRC_DIR)

EXE = demeter_gold

all: demeter_util $(EXE)

$(EXE): $(OBJS) $(UTIL)
	$(LINK.o) $(OBJS) $(LOADLIBES) $(LDLIBS) -o $@

demeter_util:
	$(MAKE) -C $(UTIL_DIR)

$(OBJS): CPPFLAGS+=-DQUERY_INDEX_KEY

clean:
	$(RM) *.o *.d $(EXE)

.PHONY: clean demeter_util
-include $(wildcard *.d)
