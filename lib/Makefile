CC = gcc
BASEFILE  = ../xscript-stdlib-1.0.0
CCFLAGS = -c -w -fPIC -I ../header
LDFLAGS = -o $(OUTFILE) -shared 
FILES=console.c file.c math.c network.c sys.c datatypes_list.c
OBJFILES=$(addsuffix .o,$(basename $(FILES)))

ifeq ($(OS),Windows_NT)
	OUTFILE += $(BASEFILE).dll
	LDFLAGS += "-s -Wl,--subsystem,windows"
else
	OUTFILE=$(BASEFILE).so
endif

all: $(OBJFILES)
	echo $(OUTFILE)
	$(CC) $(LDFLAGS) $(OBJFILES) -o $(OUTFILE)

$(OBJFILES): %.o: %.c
	$(CC) $(CCFLAGS) $< -o $@

