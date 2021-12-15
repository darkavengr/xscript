CC = gcc
LD = ld 

all:
	$(CC) -c -w addvar.c dofile.c expr.c itoa.c xscript.c
	$(CC) addvar.o dofile.o expr.o itoa.o xscript.o -oxscript -ldl

clean:
	rm xscript addvar.o dofile.o expr.o itoa.o xscript.o



