CC = gcc
OUTFILE  = xscript
FLAGS    = -lm
CCFLAGS = -c -w -I header -fstack-protector-all
FILES=debug.c module.c variablesandfunctions.c dofile.c evaluate.c itoa.c xscript.c error.c statements.c interactivemode.c help.c support.c
OBJFILES=$(addsuffix .o,$(basename $(FILES)))


ifeq ($(OS),Windows_NT)
	OUTFILE += ".exe"
	FILES += winmodule.c
else
	FILES += linux-module.c
	FLAGS += -ldl
endif

all: $(OBJFILES)
	echo $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS)

debug: $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS) -fsanitize=address

clean:
	rm *.o

$(OBJFILES): %.o: %.c
	$(CC) $(CCFLAGS) $< -o $@

