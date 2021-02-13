ODIR=./build/objs
BDIR=./build/bin

IDIR=./src
SDIR=./src

PARSED=parsed
CC=gcc
LINK=gcc
DFLAGS=-g -O0 -fsanitize=address,leak,undefined
RFLAGS=-O3
CFLAGS=-I$(IDIR) -I$(IDIR)/regex/src -Wall $(DFLAGS)

_SRC=$(wildcard $(SDIR)/regex/src/*.c) $(wildcard $(SDIR)/*.c) $(wildcard $(SDIR)/*/*.c)
_SRC+=$(SDIR)/parser/parser.c
OBJ=$(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(_SRC))

DEPS=$(wildcard $(IDIR)/regex/src/*.h) $(wildcard $(IDIR)/*.h)

_BIN=parsed
BIN=$(patsubst %,$(BDIR)/%,$(_BIN))

.PHONY: all
all: $(BIN)

$(BDIR)/$(_BIN): $(OBJ)
	mkdir -p `dirname $@`
	$(LINK) $(CFLAGS) -o $@ $^ $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/parser/parser.o: $(SDIR)/parser/parsed.grammar
	mkdir -p `dirname $@`
	$(PARSED) $< $(SDIR)/parser/parser.c
	$(CC) $(CFLAGS) -w -c -o $@ $(SDIR)/parser/parser.c
	
.PHONY: new
new: clean all
	
.PHONY: clean
clean:
	rm -fr $(ODIR)/*

.PHONY: cleanall
cleanall:
	rm -fr $(ODIR)/* $(BDIR)/*

.PHONY: check
check: all
	./build/bin/test
