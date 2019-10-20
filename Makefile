.PHONY: test clean

MAKE=make
SRC=src
TEST=test

build: src/*.c
	cd $(SRC) && $(MAKE) && cd ..

test: build
	cd $(TEST) && $(MAKE) run && cd ..

clean:
	rm -rf build