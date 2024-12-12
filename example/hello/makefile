ROOT = ../..
LIBDIR = $(ROOT)/lib
INCLUDEDIR = $(ROOT)/include


all: dynamic static

main.o: main.c
	gcc -c $< -I$(INCLUDEDIR)

dynamic: main.o
	gcc -L$(LIBDIR) -o hello.out $< -lwebc

static: main.o
	gcc --static -o hello_static.out $< -L$(LIBDIR) -lwebc

clean:
	rm -f main.o *.out
