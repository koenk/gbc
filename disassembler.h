#pragma once
#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "types.h"
#include "GBC.h"

int disassemble(GBC emu);
int disassemble(GBC emu, u16 pc);
void disassemble_bootblock(GBC emu);

#endif
