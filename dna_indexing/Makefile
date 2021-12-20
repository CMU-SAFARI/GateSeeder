SRC_DIR		:= src
CC			:= gcc

DEPFLAGS	= -MD -MP
CFLAGS		= -g -O3 $(DEPFLAGS) -Wall -Werror
LDLIBS		= -pthread -lm
VPATH		= $(SRC_DIR)
EXE			= indexdna

SRCS		= $(wildcard $(SRC_DIR)/*.c)
OBJS		= $(SRCS:$(SRC_DIR)/%.c=%.o)

all: $(EXE)

run: $(EXE)
	#./$(EXE) -p -k 18 -w 12 -b 26 -f 2000 ../res/GCF_000001405.39_GRCh38.p13_genomic.fna /tmp/test
	./$(EXE) -k 18 -w 12 -b 26 -f 2000 ../res/GCF_000001405.39_GRCh38.p13_genomic.fna /tmp/test

$(EXE): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

format:
	clang-format -i $(SRC_DIR)/*

clean:
	$(RM) *.o $(EXE) *.d

.PHONY: clean format all run
-include $(wildcard *.d)
