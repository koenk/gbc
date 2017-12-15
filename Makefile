PROGNAME = main
OBJFILES = main.o cpu.o mmu.o disassembler.o gui.o debugger.o

CC = clang
RM = rm -fv

W_FLAGS = -Wall -Wextra -Werror-implicit-function-declaration -Wshadow
CFLAGS = -MD -std=c11 -g3 -O3 $(W_FLAGS)
LFLAGS = -lm -lSDL2 -lreadline

all: $(PROGNAME)

$(PROGNAME): $(OBJFILES)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

clean:
	$(RM) $(PROGNAME) *.o *.d

-include *.d
