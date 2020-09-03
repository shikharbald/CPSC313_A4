CC = gcc
CFLAGS = -Wall -g $(shell pkg-config fuse --cflags) -O3
LDLIBS = $(shell pkg-config fuse --libs) -O3

all: fat12fs fat12test

fat12fs: fat12fs.o fat12.o
fat12test: fat12test.o fat12.o

fat12fs.o: fat12fs.c fat12.h
fat12.o: fat12.c fat12.h

clean:
	-rm -rf fat12fs fat12test fat12fs.o fat12.o fat12test.o
