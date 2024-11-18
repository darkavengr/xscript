CC = gcc
OBJFILES = debug.o module.o variablesandfunctions.o dofile.o evaluate.o itoa.o xscript.o error.o statements.o interactivemode.o help.o
OUTFILE  = xscript
FLAGS    = -lm

ifeq ($(OS),Windows_NT)
	OUTFILE += ".exe"
	OBJFILES += winmodule.o
else
	OBJFILES += linux-module.o
	FLAGS += -ldl
endif

xscript: $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS)

statements.o:
	$(CC) -c -w statements.c

variablesandfunctions.o:
	$(CC) -c -w variablesandfunctions.c

dofile.o:
	$(CC) -c -w dofile.c

evaluate.o:
	$(CC) -c -w evaluate.c

itoa.o:
	$(CC) -c -w itoa.c

module.o:
	$(CC) -c -w module.c

xscript.o:
	$(CC) -c -w xscript.c

debug.o:
	$(CC) -c -w debug.c

error.o:
	$(CC) -c -w error.c

interactivemode.o:
	$(CC) -c -w interactivemode.c

help.o:
	$(CC) -c -w help.c

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

