SRC = my_route_lookup.c my_route_lookup.h io.c io.h
CFLAGS = -lm -Wall -O3 -std=c99

all: routeLookup

routeLookup: my_route_lookup.c my_route_lookup.h io.c io.h
	gcc $(CFLAGS) $(SRC) -o my_route_lookup

.PHONY: clean

clean:
	rm -f my_route_lookup
