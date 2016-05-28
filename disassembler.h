#pragma once
#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "types.h"

int disassemble_pc(struct gb_state* state, u16 pc);
void disassemble(struct gb_state* state);
void disassemble_bootblock(struct gb_state *state);

#endif
