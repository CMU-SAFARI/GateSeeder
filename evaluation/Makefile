CC := gcc

SRC_DIR := src
SCRIPT_DIR := scripts
UTIL_DIR := ../util

CPPFLAGS += -I$(UTIL_DIR)/include
CFLAGS = -std=gnu11 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror

EVICT = page_cache_evict
GEN_RES = gen_res
SIM = sim.fastq

VPATH = $(SRC_DIR)

all: $(EVICT) $(SIM) gen_res

results: $(GEN_RES) accuracy performances
	./$(GEN_RES) accuracy_demeter.txt accuracy_mm2.txt performances_demeter.txt performances_mm2.txt > results.dat

$(GEN_RES): src/gen_res.rs
	rustc $<

performances: $(EVICT)
	$(MAKE) -C ..
	./$(SCRIPT_DIR)/$@.sh

accuracy: $(SIM)
	$(MAKE) -C ..
	./$(SCRIPT_DIR)/$@.sh

$(SIM):
	./$(SCRIPT_DIR)/gen_sim.sh

$(EVICT): src/$(EVICT).c


clean:
	$(RM) $(EVICT) $(SIM) $(GEN_RES)


.PHONY: all clean accuracy performances results
