SRC_DIR		:= src
CC			:= gcc

DEPFLAGS	= -MD -MP
NB_THREADS	= 8
CFLAGS		= -g -O3 $(DEPFLAGS) -Wall -Werror -DNB_THREADS=$(NB_THREADS) -DMULTI_THREAD
LDLIBS		= -pthread
VPATH		= $(SRC_DIR)
SRCS		= $(wildcard $(SRC_DIR)/*.c)

PFLAGS 		= -DVARIABLE_LEN
IFLAGS 		=

IEXE			= map-illumina
PEXE			= map-pacbio

POBJS		= $(SRCS:$(SRC_DIR)/%.c=pacbio_obj/%.o)
IOBJS		= $(SRCS:$(SRC_DIR)/%.c=illumina_obj/%.o)

all: $(IEXE) $(PEXE)

$(IEXE): $(IOBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(PEXE): $(POBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

illumina_obj/%.o: $(SRC_DIR)/%.c | illumina_obj
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

pacbio_obj/%.o: $(SRC_DIR)/%.c | pacbio_obj
	$(CC) $(CFLAGS) $(PFLAGS) -c $< -o $@

format:
	clang-format -i $(SRC_DIR)/*

clean:
	$(RM) -r $(IEXE) $(PEXE) illumina_obj pacbio_obj

illumina_obj:
	mkdir $@

pacbio_obj:
	mkdir $@

run_i: $(IEXE)
	./$^ -i ../res/full_k18_w12_f1000_b26.bin ../res/ERR240727_1.bin -o /tmp/locs
run_p: $(PEXE)
	./$^ -i ../res/full_k18_w12_f1000_b26.bin ../res/pacbio_10000.bin -o /tmp/locs

.PHONY: clean format all run_i run_p test
-include $(wildcard illumina_obj/*.d) $(wildcard pacbio_obj/*.d)
