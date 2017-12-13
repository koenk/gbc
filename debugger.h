#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "types.h"

void dbg_print_regs(struct gb_state* s);
int dbg_run_debugger(struct gb_state *s);

#endif
