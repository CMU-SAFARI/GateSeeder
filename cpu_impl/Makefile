SRC_DIR		:= src
CC			:= gcc

DEPFLAGS	= -MD -MP
NB_THREADS	= 8
CFLAGS		= -g -O3 $(DEPFLAGS) -Wall -Werror -DNB_THREADS=$(NB_THREADS) -DVARIABLE_LEN -DMULTI_THREAD
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

run: $(EXE)
	#./$^ -i ../res/full_k18_w12_f1000_b26.bin ../res/ERR240727_1.bin -o /tmp/locs
	./$^ -i ../res/full_k18_w12_f1000_b26.bin ../res/pacbio_10000.bin -o /tmp/locs

.PHONY: clean format all run test
-include $(wildcard *.d)
