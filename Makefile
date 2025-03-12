#
# file:        Makefile - project 2
# description: compile, link with pthread and zlib (crc32) libraries
#

LDLIBS=-lz -lpthread
CFLAGS=-ggdb3 -Wall -Wno-format-overflow

EXES = dbserver dbtest

all: $(EXES)

dbtest.o: dbtest.c
	gcc $(CFLAGS) -c $<
dbserver.o: dbserver.c
	gcc $(CFLAGS) -c $<
dbtest: dbtest.o
	gcc $(CFLAGS) $< -o $@ $(LDLIBS)
dbserver: dbserver.o
	gcc $(CFLAGS) $< -o $@ $(LDLIBS)
clean:
	rm -f $(EXES) *.o data.[0-9]*
