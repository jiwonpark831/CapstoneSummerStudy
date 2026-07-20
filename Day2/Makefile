CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET_CLI = day2_client
TARGET_SRV = day2_server

SRCS_CLI = day2_client.c
SRCS_SRV = day2_server.c

all: $(TARGET_CLI) $(TARGET_SRV)

$(TARGET_CLI): $(SRCS_CLI)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_SRV): $(SRCS_SRV)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET_CLI) $(TARGET_SRV)

.PHONY: all clean