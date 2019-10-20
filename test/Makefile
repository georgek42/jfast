.PHONY: run clean

CC=gcc
FLAGS=-O0 -g
SOURCES=*.c
LIB=../build
DBG=lldb

test: test.c
	$(CC) $(FLAGS) $(SOURCES) -L$(LIB) -ljfast -o test

run: test
	./test cases/*.json

debug:
	$(DBG) test -- $(CASE)

clean:
	rm -f test