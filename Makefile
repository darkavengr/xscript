CC = gcc
BASEFILE  = xscript
FLAGS    = -lm
CCFLAGS = -c -w -I header -fstack-protector-all
FILES=debug.c module.c variablesandfunctions.c dofile.c evaluate.c itoa.c xscript.c error.c statements.c interactivemode.c help.c support.c
OBJFILES=$(addsuffix .o,$(basename $(FILES)))

ifeq ($(OS),Windows_NT)
	OUTFILE=$(BASEFILE).exe
	FILES += winmodule.c
else
	OUTFILE=$(BASEFILE)
	FILES += linux-module.c
	FLAGS += -ldl
endif

all: interpreter stdlib

interpreter: $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS)

stdlib:
	make -C lib

debug: $(OBJFILES)
	$(CC) $(OBJFILES) -o $(OUTFILE) $(FLAGS) -fsanitize=address

clean:
	rm *.o lib/*.o $(OUTFILE) xscript-stdlib-1.0.0.so

$(OBJFILES): %.o: %.c
	$(CC) $(CCFLAGS) $< -o $@

