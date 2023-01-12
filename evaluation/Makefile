CC := gcc

SRC_DIR := src
UTIL_DIR := ../util

CPPFLAGS += -I$(UTIL_DIR)/include
CFLAGS = -std=gnu11 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror

EVICT: page_cache_evict

VPATH = $(SRC_DIR)

all: $(EVICT)
$(EVICT): src/$(EVICT).c

clean:
	$(RM) page_cache_evict


.PHONY: all clean
