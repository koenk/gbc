[![CI](https://github.com/koenk/gbc/actions/workflows/ci.yml/badge.svg)](https://github.com/koenk/gbc/actions/workflows/ci.yml)
[![CD](https://github.com/koenk/gbc/actions/workflows/cd.yml/badge.svg)](https://github.com/koenk/gbc/actions/workflows/cd.yml)

# GBC

A simple GameBoy and GameBoy Color emulator written in C.

This is a hobby project to try and write a simple emulator for the GameBoy,
including its CPU, memory controller and display. It seems to work well enough
with a number of games, but has not been extensively tested. If you want an
actual practical GameBoy emulator you should use something different, there are
plenty of mature emulators out there (e.g., Gambatte, BGB, VBA).

The emulator supports all opcodes of the GameBoy CPU (and passes the GameBoy
tests from Blargg). It supports all areas of the GameBoy's MMU, including
a number of different (on-cartridge) memory controllers (MBC-1, MBC-3 and
MBC-5). It supports most of the LCD functionality (barring some sprite priority
issues).  A very hacky preliminary version of audio is implemented (supporting
only some features of a single channel, 2). While audio is recognizable (e.g.,
the Tetris music), it's limited and its timing is off, making it rather
unpleasant to listen to.

The emulator is relatively platform and architecture independent, and can be
built using different front-ends: a standalone one using SDL2 and as a libretro
core. libretro is a generic framework for emulators and runs on numerous
systems. Using this frontend for this emulator allows it to even run on the SNES
mini without modification (using hakchi2/Retroarch).

## Building and usage

Any C compiler should work, but the emulator is currently tested with Clang and
GCC. The standalone frontend depends on libreadline and libsdl2, available on
Ubuntu with by installing `libreadline-dev` and `libsdl2-dev`. For the libretro
core there are no dependencies for building, but using it requires a compatible
frontend such as Retroarch.

Building and running the standalone frontend can be done as such:
```shell
make
./main path/to/romfile
```

Running `./main -h` shows all available options. Button mappings are as follows:

GameBoy button | Keyboard
-------------- | --------
A              | z
B              | x
Directions     | Arrow keys
Start          | Enter
Select         | Backspace
*Quit*         | q or Escape
*Save state*   | s
*Break*        | b

When the emulator detects unexpected behavior (e.g., accessing an unknown memory
region), it will drop into a built-in debugger. This debugger can also be
invoked manually using the `b` key. The debugger allows inspection of code, data
and registers, can single-step the execution and set breakpoints, and can toggle
debug-prints for instructions and memory accesses at run-time. For a list of
commands, see the `h` command.


## As a libretro core

To build just the libretro core, use the `make libretro` command, which will
create `koengb_libretro.so`. By placing this shared library in the cores
directory of a frontend such as Retroarch (`~/.config/retroarch/cores`) it can
be selected from the interface.

### Using the libretro core on the SNES mini

**Note:** There are much better and more mature emulators already available for
the SNES mini.

Since the SNES mini (Super Nintendo Classic Mini) is simply an ARM chip running
Linux, which can run Retroarch via hakchi2, we can run our libretro core on
there as well simply by cross-compiling the code. This requires an ARM
cross-compiler, which can be found on Ubuntu in the `gcc-arm-linux-gnueabihf`
package. Then compiling can be done as such:
```shell
make clean  # To clean up any .o files created for the native architecture
make libretro CC=arm-linux-gnueabihf-gcc
```
Copy `koengb_libretro.so` to the SNES mini with hakchi2's FTP in
`/var/lib/hakchi/rootfs/etc/libretro/core`. Create a folder in the games
directory (`/var/lib/hakchi/rootfs/usr/share/games/001`), and modify the
`.desktop` file in there to invoke Retroarch with this frontend (e.g.,
`Exec=/bin/retroarch-clover koengb path/to/rom`).

Currently the libretro support is limited, so exiting out of the emulator using
the reset button, and managing states does not work yet.
