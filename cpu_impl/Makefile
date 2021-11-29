SRC_DIR		:= src
CC			:= gcc

DEPFLAGS	= -MD -MP
CFLAGS		= -g -O3 $(DEPFLAGS) -Wall -Werror -DMULTI_THREAD -DNB_THREADS=8
LDLIBS		= -pthread
VPATH		= $(SRC_DIR)
EXE			= seeding

SRCS		= $(wildcard $(SRC_DIR)/*.c)
OBJS		= $(SRCS:$(SRC_DIR)/%.c=%.o)

all: $(EXE)

$(EXE): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

format:
	clang-format -i $(SRC_DIR)/*

clean:
	$(RM) *.o $(EXE) *.d

test: $(EXE)
	./$^ -k 18 -w 12 -b 26 -m 3 -l 150 -i ../res/chr1_k18_w12_f700_b26.bin ../res/ERR240727_1.fastq -c ../res/illumina_k18_w12.paf

run: $(EXE)
	./$^ -i ../res/full_k18_w12_f1000_b26.bin ../res/ERR240727_1.fastq -o /tmp/locs

.PHONY: clean format all run test
-include $(wildcard *.d)
