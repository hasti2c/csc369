CC = gcc
CFLAGS := -g3 -Wall -Wextra -Werror $(CFLAGS)
LDFLAGS := $(LDFLAGS)

.PHONY: all clean

all: sim

sim: rr.o rand.o lru.o clock.o pagetable.o sim.o swap.o
	$(CC) $^ -o $@ $(LDFLAGS)

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:.c=.o)

-include $(OBJ_FILES:.o=.d)

%.o: %.c
	$(CC) $< -o $@ -c -MMD $(CFLAGS)

clean:
	rm -f $(OBJ_FILES) $(OBJ_FILES:.o=.d) sim swapfile.*
