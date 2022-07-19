CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -Wpedantic -ggdb
CLIBS=-lm

OBJDIR=lib
CFILES=main.c png.c
OBJFILES=main.o png.o
OBJ=$(addprefix $(OBJDIR)/, $(OBJFILES))

BIN=main

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CLIBS) -o $@ $^

$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BIN) $(OBJDIR) out.png rand_*.png
