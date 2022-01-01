CC = gcc
LD = ld 

all:

ifeq ($(OS),Windows_NT)
	$(CC) -c -w addvar.c dofile.c expr.c itoa.c winmodule.c xscript.c
	$(CC) addvar.o dofile.o expr.o itoa.o winmodule.c xscript.o -oxscript.exe
else
	$(CC) -c -w addvar.c dofile.c expr.c itoa.c linux-module.c xscript.c
	$(CC) addvar.o dofile.o expr.o itoa.o xscript.o linux-module.o -oxscript -ldl
endif

clean:
	rm addvar.o dofile.o expr.o itoa.o xscript.o
ifeq ($(OS),Windows_NT)
	rm winmodule.o xscript.exe
else
	rm linux-module.o xscript
endif




