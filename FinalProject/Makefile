CC = gcc

CFLAGS = -Wall -g

TARGETS = p2p

all: $(TARGETS)

p2p: p2p.c
	$(CC) $(CFLAGS) -o p2p p2p.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean