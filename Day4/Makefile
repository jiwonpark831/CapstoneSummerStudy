CC = gcc

CFLAGS = -Wall -g

TARGETS = day4_client day4_server

all: $(TARGETS)

client: day4_client.c
	$(CC) $(CFLAGS) -o day4_client day4_client.c

server: day4_server.c
	$(CC) $(CFLAGS) -o day4_server day4_server.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean