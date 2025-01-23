all: assemble

assemble: assemble16-linux.c
	gcc -Wall -pedantic -O2 -o assemble assemble16-linux.c

.PHONY: clean
clean:
	rm -rf assemble
