CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET_CLI = day1_client
TARGET_SRV = day1_server

SRCS_CLI = day1_client.c
SRCS_SRV = day1_server.c

all: $(TARGET_CLI) $(TARGET_SRV)

$(TARGET_CLI): $(SRCS_CLI)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_SRV): $(SRCS_SRV)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET_CLI) $(TARGET_SRV)

.PHONY: all clean