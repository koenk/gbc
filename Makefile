PROGNAME = main
OBJFILES = main.o cpu.o mmu.o disassembler.o

CC = clang
RM = rm -fv

W_FLAGS = -Wall -Wextra -Werror-implicit-function-declaration -Wshadow -pedantic-errors
CFLAGS = -MD -std=c11 -g -O2 $(W_FLAGS)
LFLAGS = -lm

all: $(PROGNAME)

$(PROGNAME): $(OBJFILES)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

clean:
	$(RM) $(PROGNAME) *.o

-include *.d
