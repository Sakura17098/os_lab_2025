CC = gcc
CFLAGS = -Wall -Wextra -std=c11
PROGRAMS = build/tcpclient build/tcpserver build/udpclient build/udpserver

all: $(PROGRAMS)

build:
	mkdir -p build

build/tcpclient: src/tcpclient.c | build
	$(CC) $(CFLAGS) -o $@ $<

build/tcpserver: src/tcpserver.c | build
	$(CC) $(CFLAGS) -o $@ $<

build/udpclient: src/udpclient.c | build
	$(CC) $(CFLAGS) -o $@ $<

build/udpserver: src/udpserver.c | build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf build

.PHONY: all clean
