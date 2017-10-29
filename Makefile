all: aring.o

CFLAGS=-Os -std=c11

asm: aring.s

aring.s:
	cc -S -std=c11 -O3 aring.c

clean:
	rm aring.[os]
