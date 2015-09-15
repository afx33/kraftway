CC = g++
CFLAGS=-Wall -ggdb -g3 -std=c++0x -std=c++11

#CFLAGS=-Wall -ggdb -g3 -std=c++11

all: sort_file

sort_file: main.o
	$(CC) $(CFLAGS) main.o -o sort_file

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean:
	rm -rf *.o sort_file sorted 
	find * -maxdepth 0 -name 'chunk*' -exec rm '{}' ';'

