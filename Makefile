CC := gcc
CFLAGS := -Wall -Wextra -MMD -MP -I.

SRC := my_malloc.c test_my_malloc.c
OBJ := $(SRC:%.c=%.o)
DEPS := $(OBJ:.o=.d)

all: test

test: $(OBJ)
	$(CC) $^ -o test

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(DEPS) test

.PHONY: all clean test

-include $(DEPS)
