CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = 

TARGET_SENDER = sender
TARGET_RECEIVER = receiver

HDRS = p2p.h
COMMON_SRCS = php.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

SENDER_SRCS = sender.c $(COMMON_SRCS)
SENDER_OBJS = $(SENDER_SRCS:.c=.o)

RECEIVER_SRCS = receiver.c $(COMMON_SRCS)
RECEIVER_OBJS = $(RECEIVER_SRCS:.c=.o)

all: $(TARGET_SENDER) $(TARGET_RECEIVER)

$(TARGET_SENDER): $(SENDER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_RECEIVER): $(RECEIVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET_SENDER) $(TARGET_RECEIVER)

.PHONY: all clean