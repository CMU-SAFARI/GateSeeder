SRC_DIR		:= src
CC			:= gcc

DEPFLAGS	= -MD -MP
CFLAGS		= -g -O3 $(DEPFLAGS) -Wall #-Werror
LDLIBS		=
VPATH		= $(SRC_DIR)
EXE			= fastq2bin

SRCS		= $(wildcard $(SRC_DIR)/*.c)
OBJS		= $(SRCS:$(SRC_DIR)/%.c=%.o)

all: $(EXE)

$(EXE): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

format:
	clang-format -i $(SRC_DIR)/*

clean:
	$(RM) *.o $(EXE) *.d

run: $(EXE)
	./$(EXE) -l 100 ../res/illumina_1000.fastq ../res/illumina_1000
	./$(EXE) -v -l 20000 ../res/pacbio_100.fastq ../res/pacbio_100

.PHONY: clean format all run
-include $(wildcard *.d)
