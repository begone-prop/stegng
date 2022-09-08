CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -Wpedantic -O2
CLIBS=-lm

OBJDIR=lib
CFILES=cpng.c png.c
OBJFILES=cpng.o png.o
OBJ=$(addprefix $(OBJDIR)/, $(OBJFILES))

BIN=cpng

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CLIBS) -o $@ $^

$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BIN) $(OBJDIR) out.png rand_*.png
