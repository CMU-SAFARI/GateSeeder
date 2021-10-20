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
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4}" $(SRC_DIR)/*

clean:
	$(RM) *.o $(EXE) *.d

run: $(EXE)
	./main -i ../res/u_w12_k18_f500_b26.bin ../res/ERR240727_1.fastq -c ../res/u_w12_k18.paf -m 3 -b26

.PHONY: clean format all run
-include $(wildcard *.d)
