CC = gcc
OBJFILES = debug.o module.o variablesandfunctions.o dofile.o evaluate.o itoa.o xscript.o error.o statements.o interactivemode.o help.o
OUTFILE  = xscript
FLAGS    = -lm
CCFLAGS = -c -w -fstack-protector-all -fsanitize=address

ifeq ($(OS),Windows_NT)
	OUTFILE += ".exe"
	OBJFILES += winmodule.o
else
	OBJFILES += linux-module.o
	FLAGS += -ldl
endif

all: $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS) -fsanitize=address


statements.o:
	$(CC) $(CCFLAGS) statements.c

variablesandfunctions.o:
	$(CC) $(CCFLAGS) variablesandfunctions.c

dofile.o:
	$(CC) $(CCFLAGS) dofile.c

evaluate.o:
	$(CC) $(CCFLAGS) evaluate.c

itoa.o:
	$(CC) $(CCFLAGS) itoa.c

module.o:
	$(CC) $(CCFLAGS) module.c

xscript.o:
	$(CC) $(CCFLAGS) xscript.c

debug.o:
	$(CC) $(CCFLAGS) debug.c

error.o:
	$(CC) $(CCFLAGS) error.c

interactivemode.o:
	$(CC) $(CCFLAGS) interactivemode.c

help.o:
	$(CC) $(CCFLAGS) help.c

ifeq ($(OS),Windows_NT)
winmodule.o:
	$(CC) $(CCFLAGS) winmodule.c

else
linux-module.o:
	$(CC) $(CCFLAGS) linux-module.c
endif

clean:
	rm *.o
ifeq ($(OS),Windows_NT)
	rm xscript.exe
else
	rm xscript
endif

