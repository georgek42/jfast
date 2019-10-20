CC=gcc
FLAGS=-O0 -g
SOURCES=*.c
BUILD=../build

libjfast.so: jfast.c
	mkdir -p $(BUILD)
	$(CC) $(FLAGS) $(SOURCES) -shared -o $(BUILD)/libjfast.so
