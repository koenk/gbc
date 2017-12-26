PROGNAME = main
OBJFILES = main.o state.o cpu.o mmu.o disassembler.o sdl.o debugger.o lcd.o \
		   audio.o

CC = clang
RM = rm -fv

W_FLAGS = -Wall -Wextra -Werror-implicit-function-declaration -Wshadow
CFLAGS = -MD -std=c11 -g3 -O0 $(W_FLAGS) -fsanitize=address
LDFLAGS = -g3 -lm -lSDL2 -lreadline -fsanitize=address

all: $(PROGNAME)

$(PROGNAME): $(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(PROGNAME) *.o *.d

-include *.d
