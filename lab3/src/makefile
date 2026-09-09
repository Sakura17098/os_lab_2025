CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -I.

.PHONY: all clean

all: sequential_min_max parallel_min_max exec_sequential_min_max

sequential_min_max: sequential_min_max.c utils.o find_min_max.o utils.h find_min_max.h
	$(CC) $(CFLAGS) -o sequential_min_max sequential_min_max.c find_min_max.o utils.o

parallel_min_max: parallel_min_max.c utils.o find_min_max.o utils.h find_min_max.h
	$(CC) $(CFLAGS) -o parallel_min_max parallel_min_max.c find_min_max.o utils.o

exec_sequential_min_max: exec_sequential_min_max.c
	$(CC) $(CFLAGS) -o exec_sequential_min_max exec_sequential_min_max.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c -o utils.o

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c find_min_max.c -o find_min_max.o

clean:
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max exec_sequential_min_max min_max_*.txt
