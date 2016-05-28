#pragma once
#ifndef GBC_H
#define GBC_H

#include "types.h"

void cpu_reset_state(struct gb_state* s);
void cpu_print_header_info(struct gb_state* s);
void cpu_print_regs(struct gb_state* s);
int cpu_do_instruction(struct gb_state* s);



#endif
