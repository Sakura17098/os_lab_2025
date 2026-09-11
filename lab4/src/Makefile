CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -Werror -std=c11 -I. -pthread

TARGETS = parallel_min_max zombie_demo process_memory psum
LIBRARY = libsum.a

all: $(TARGETS)

parallel_min_max: parallel_min_max.o find_min_max.o utils.o
	$(CC) $(CFLAGS) parallel_min_max.o find_min_max.o utils.o -o parallel_min_max

parallel_min_max.o: parallel_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c parallel_min_max.c -o parallel_min_max.o

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c find_min_max.c -o find_min_max.o

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c -o utils.o

zombie_demo: zombie_demo.c
	$(CC) $(CFLAGS) zombie_demo.c -o zombie_demo

process_memory: process_memory.c
	$(CC) $(CFLAGS) process_memory.c -o process_memory

psum: parallel_sum.o $(LIBRARY) utils.o
	$(CC) $(CFLAGS) parallel_sum.o utils.o -L. -lsum -o psum

parallel_sum.o: parallel_sum.c sum.h utils.h
	$(CC) $(CFLAGS) -c parallel_sum.c -o parallel_sum.o

sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -c sum.c -o sum.o

$(LIBRARY): sum.o
	$(AR) rcs $(LIBRARY) sum.o

clean:
	rm -f *.o parallel_min_max zombie_demo process_memory psum $(LIBRARY) min_max_*.txt

.PHONY: all clean
