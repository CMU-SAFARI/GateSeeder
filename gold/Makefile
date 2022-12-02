
SRC_DIR		:= src
SRCS		= $(wildcard $(SRC_DIR)/*.c)
OBJS		= $(SRCS:$(SRC_DIR)/%.c=%.o)

DEPFLAGS	= -MD -MP
CXXFLAGS	= -O3 -g $(DEPFLAGS) -Wall -Wextra -Werror
VPATH		=$(SRC_DIR)

GOLD			= gold.x

$(GOLD): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) *.o *.d $(GOLD)

.PHONY: clean
-include $(wildcard *.d)
