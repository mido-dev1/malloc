CC := gcc
CFLAGS := -Wall -Wextra -MMD -MP -I. -fPIC

SRC_TEST := my_malloc.c test_my_malloc.c
OBJ_TEST := $(SRC_TEST:%.c=%.o)
DEPS_TEST := $(OBJ_TEST:.o=.d)

SRC := my_malloc.c
OBJ := $(SRC:%.c=%.o)

STATIC := libmy_malloc.a
SHARED := libmy_malloc.so

all: test

test: $(OBJ)
	$(CC) $^ -o test

static: $(STATIC)
$(STATIC): $(OBJ)
	ar rcs $@ $^

shared: $(SHARED)
$(SHARED): $(OBJ)
	$(CC) -shared -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(OBJ_TEST) $(DEPS_TEST)

.PHONY: all clean test static shared

-include $(DEPS_TEST)
