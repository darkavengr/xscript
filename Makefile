CC = gcc
OBJFILES=module.o addvar.o dofile.o expr.o itoa.o xscript.o
OUTFILE=xscript

ifeq ($(OS),Windows_NT)
	OUTFILE += ".exe"
	OBJFILES += winmodule.o
else
	OBJFILES += linux-module.o
	FLAGS = -ldl
endif

xscript: $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS)

addvar.o:
	$(CC) -c -w addvar.c

dofile.o:
	$(CC) -c -w dofile.c

expr.o:
	$(CC) -c -w expr.c

itoa.o:
	$(CC) -c -w itoa.c

module.o:
	$(CC) -c -w module.c

xscript.o:
	$(CC) -c -w xscript.c

ifeq ($(OS),Windows_NT)
winmodule.o:
	$(CC) -c -w winmodule.c

else
linux-module.o:
	$(CC) -c -w linux-module.c
endif

clean:
	rm *.o
ifeq ($(OS),Windows_NT)
	rm xscript.exe
else
	rm xscript
endif




