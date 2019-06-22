SRC = rib.c rib.h io.c io.h main.c main.h
CFLAGS = -Wall -O3 -std=c99

all: routeLookup

routeLookup: rib.c rib.h io.c io.h main.c main.h 
	gcc $(CFLAGS) $(SRC) -o rib_test -lm

.PHONY: clean

clean:
	rm -f rib_test
