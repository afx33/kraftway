CC = g++
CFLAGS=-Wall -ggdb -g3 -std=c++0x -std=c++11

all: file_creator

file_creator: main.o
	$(CC) $(CFLAGS) main.o -o file_creator

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean:
	rm -rf *.o file_creator unsorted

