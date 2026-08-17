CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = p2p

SRCS = p2p.c sender.c receiver.c
OBJS = $(SRCS:.c=.o)
HEADERS = p2p.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean