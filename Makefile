PROGNAME = GBC
OFILES = main.o GBC.o

CC = clang++ # or g++
RM = rm -fv

W_FLAGS = -Wall -Wextra -Werror-implicit-function-declaration -Wshadow -pedantic-errors
CFLAGS = -g -O2 $(W_FLAGS)
LFLAGS = -lm

all: $(PROGNAME)

$(PROGNAME): $(OFILES)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) $(PROGNAME) *.o

