SRC_DIR		:= src

CC			:= gcc
DEPFLAGS	= -MD -MP
CFLAGS		= -std=gnu11 -g -O3 $(DEPFLAGS) -Wall -Werror -Wextra
LDLIBS		= -pthread

SRCS		= $(wildcard $(SRC_DIR)/*.c)
OBJS		= $(SRCS:$(SRC_DIR)/%.c=%.o)

VPATH		= src

EXE			:= aloha_index

all: $(EXE)

$(EXE): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) *.o *.d $(EXE)

.PHONY: all clean
-include $(wildcard *.d)
