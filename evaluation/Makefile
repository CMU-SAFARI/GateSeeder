CC := gcc

SRC_DIR := src
SCRIPT_DIR := scripts
UTIL_DIR := ../util

CPPFLAGS += -I$(UTIL_DIR)/include
CFLAGS = -std=gnu11 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror

EVICT = page_cache_evict
SIM = sim.fastq

VPATH = $(SRC_DIR)

all: $(EVICT) $(SIM)

accuracy: $(SIM)
	./$(SCRIPT_DIR)/accuracy.sh

$(SIM):
	./$(SCRIPT_DIR)/gen_sim.sh

$(EVICT): src/$(EVICT).c


clean:
	$(RM) $(EVICT) $(SIM)


.PHONY: all clean accuracy
