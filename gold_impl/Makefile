SRC_DIR		:= src
CC		:= gcc

DEPFLAGS	= -MD -MP
CFLAGS		= -g -O3 $(DEPFLAGS) -Wall -Werror
LDLIBS		= -pthread -lm
VPATH		= $(SRC_DIR)
EXE		= main

SRCS		= $(wildcard $(SRC_DIR)/*.c)
OBJS		= $(SRCS:$(SRC_DIR)/%.c=%.o)

all: $(EXE)

$(EXE): $(OBJS)

format:
	clang-format -i $(SRC_DIR)/*

clean:
	$(RM) *.o $(EXE) *.d

test: $(EXE)
	./main -k 18 -w 12 -b 26 -m 3 -l 150 -i ../res/chr1_k18_w12_f700_b26.bin ../res/ERR240727_1.fastq -c ../res/illumina_k18_w12.paf

run: $(EXE)
	./main -k 18 -w 12 -b 26 -m 3 -l 150 -i ../res/chr1_k18_w12_f700_b26.bin ../res/illumina_100.fastq > ../res/illumina_100_chr1_k18_w12_f700_b26_m3_l150.dat

idx: $(EXE)
	./main -k 18 -w 12 -f 1000 -b 26 ../res/GCF_000001405.39_GRCh38.p13_genomic.fna ../res/full_k18_w12_f1000_b26.bin -o

.PHONY: clean format all run test idx
-include $(wildcard *.d)
