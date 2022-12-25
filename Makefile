CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -Wpedantic -O2
CLIBS=-lm

OBJDIR=lib
CFILES=stegng.c png.c
OBJFILES=stegng.o png.o
OBJ=$(addprefix $(OBJDIR)/, $(OBJFILES))

BIN=stegng

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CLIBS) -o $@ $^

$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BIN) $(OBJDIR) out.png rand_*.png
