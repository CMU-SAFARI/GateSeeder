SRC_DIR		:= src
SRCS		= $(wildcard $(SRC_DIR)/*.c)
OBJS		= $(SRCS:$(SRC_DIR)/%.c=%.o)

DEPFLAGS	= -MD -MP
CFLAGS	= -O3 -g $(DEPFLAGS) -Wall -Wextra -Werror
VPATH		=$(SRC_DIR)

GOLD			= gold.x

$(GOLD): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

#$(OBJS): CPPFLAGS=-DDEBUG_QUERY_INDEX_MAP -DDEBUG_SEED_EXTRACTION
$(OBJS): CPPFLAGS=-DDEBUG_SEED_EXTRACTION

clean:
	$(RM) *.o *.d $(GOLD)

.PHONY: clean
-include $(wildcard *.d)
