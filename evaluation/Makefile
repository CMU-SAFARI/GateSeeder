CC := gcc

SRC_DIR := src
SCRIPT_DIR := scripts
UTIL_DIR := ../util

CPPFLAGS += -I$(UTIL_DIR)/include
CFLAGS = -std=gnu11 -O3 -g -march=native $(DEPFLAGS) -Wall -Wextra -Werror

EVICT = page_cache_evict
GEN_RES = gen_res

VPATH = $(SRC_DIR)

all: $(GEN_RES) $(EVICT)

results_illumina.dat: $(GEN_RES) $(EVICT)
	$(MAKE) -C ..
	./$(SCRIPT_DIR)/gen_sim.sh illumina
	./$(SCRIPT_DIR)/accuracy.sh illumina
	./$(SCRIPT_DIR)/performances.sh illumina
	./$(GEN_RES) accuracy_demeter.txt accuracy_mm2.txt performances_demeter.txt performances_mm2.txt > $@

results_hifi.dat: $(GEN_RES) $(EVICT)
	$(MAKE) -C ..
	./$(SCRIPT_DIR)/gen_sim.sh hifi
	./$(SCRIPT_DIR)/accuracy.sh hifi
	./$(SCRIPT_DIR)/performances.sh hifi
	./$(GEN_RES) accuracy_demeter.txt accuracy_mm2.txt performances_demeter.txt performances_mm2.txt > $@

results_ont.dat: $(GEN_RES) $(EVICT)
	$(MAKE) -C ..
	./$(SCRIPT_DIR)/gen_sim.sh ont
	./$(SCRIPT_DIR)/accuracy.sh ont
	./$(SCRIPT_DIR)/performances.sh ont
	./$(GEN_RES) accuracy_demeter.txt accuracy_mm2.txt performances_demeter.txt performances_mm2.txt > $@

$(GEN_RES): src/gen_res.rs
	rustc $<

$(EVICT): src/$(EVICT).c

clean:
	$(RM) $(EVICT) $(GEN_RES)


.PHONY: all clean
