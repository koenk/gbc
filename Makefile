PROGNAME = GBC
CFILES = main.cpp GBC.cpp

CC = g++
W_FLAGS = -Wall -Wextra -Werror-implicit-function-declaration -Wshadow -pedantic-errors
CFLAGS = -g -O2 -lm $(W_FLAGS)
RM = rm -f

all: $(PROGNAME)
$(PROGNAME): $(CFILES)
	 $(CC) $(CFLAGS) $(CFILES) -o $(PROGNAME)

clean:
	$(RM) $(PROGNAME)


