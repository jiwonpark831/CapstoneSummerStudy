CC = gcc

CFLAGS = -Wall -g

TARGETS = day3_client day3_server

all: $(TARGETS)

client: day3_client.c
	$(CC) $(CFLAGS) -o day3_client day3_client.c

server: day3_server.c
	$(CC) $(CFLAGS) -o day3_server day3_server.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean