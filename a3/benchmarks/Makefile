CC = gcc
CFLAGS := -g3 -Wall -Wextra -Werror $(CFLAGS)
LDFLAGS := $(LDFLAGS)

.PHONY: all clean clean-traces

PROGS = simpleloop repeatloop matmul blocked

all: $(PROGS)

simpleloop: simpleloop.c marker.h
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

#NOTE: All programs except simpleloop are compiled with -O1 so that
#      local variables in their loops are stored in CPU registers.
#      Otherwise the results for some of the replacement algorithms
#      are skewed due to the large number of stack variable accesses.

repeatloop: repeatloop.c marker.h
	$(CC) $< -o $@ -O1 $(CFLAGS) $(LDFLAGS)

matmul: matmul.c marker.h timer.h
	$(CC) $< -o $@ -O1 $(CFLAGS) $(LDFLAGS)

blocked: blocked.c marker.h timer.h
	$(CC) $< -o $@ -O1 $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROGS)
