PROGNAME = main
LIBRETRONAME = koengb_libretro.so
OBJS = emu.o state.o cpu.o mmu.o disassembler.o lcd.o audio.o fileio.o
OBJS_STANDALONE = main.o sdl.o debugger.o
OBJS_LIBRETRO = libretro.o debugger-dummy.o

OBJS_STANDALONE := $(patsubst %.o,obj_standalone/%.o,$(OBJS) $(OBJS_STANDALONE))
OBJS_LIBRETRO   := $(patsubst %.o,obj_libretro/%.o,$(OBJS) $(OBJS_LIBRETRO))

RM = rm -fv

ARCH  	 := $(shell uname -m)
UNAME_S  := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
  PLATFORM := linux
else ifeq ($(UNAME_S),Darwin)
  PLATFORM := darwin
else ifeq ($(OS),Windows_NT)
  PLATFORM := windows
else
  PLATFORM := unknown
endif

SDL2_CFLAGS  := $(shell pkg-config --cflags sdl2)
SDL2_LDFLAGS := $(shell pkg-config --libs sdl2)

W_FLAGS = -Wall -Wextra -Werror-implicit-function-declaration -Wshadow
CFLAGS = -MD -std=c11 -g3 -O0 $(W_FLAGS)
CFLAGS_STANDALONE = $(SDL2_CFLAGS)
CFLAGS_LIBRETRO = -fPIC

LDFLAGS = -g3
LDFLAGS_STANDALONE = $(SDL2_LDFLAGS) -lreadline
LDFLAGS_LIBRETRO = -fPIC -shared

.SUFFIXES: # Disable builtin rules
.PHONY: all standalone libretro clean

all: standalone libretro
standalone: $(PROGNAME)
libretro: $(LIBRETRONAME)

$(PROGNAME): $(OBJS_STANDALONE)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDFLAGS_STANDALONE)

$(LIBRETRONAME): $(OBJS_LIBRETRO)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDFLAGS_LIBRETRO)

obj_libretro:
	mkdir -p $@
obj_libretro/%.o: %.c | obj_libretro
	$(CC) $(CFLAGS) $(CFLAGS_LIBRETRO) -c -o $@ $<

obj_standalone:
	mkdir -p $@
obj_standalone/%.o: %.c | obj_standalone
	$(CC) $(CFLAGS) $(CFLAGS_STANDALONE) -c -o $@ $<

clean:
	$(RM) $(PROGNAME) $(LIBRETRONAME)
	$(RM) -r obj_standalone obj_libretro

zip:
	zip -9 -r gbc-${PLATFORM}-${ARCH}.zip ${PROGNAME}
	zip -9 -r koengb_libretro-${PLATFORM}-${ARCH}.zip ${LIBRETRONAME}

-include obj_standalone/*.d
-include obj_libretro/*.d
