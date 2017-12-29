#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "debugger.h"
#include "disassembler.h"
#include "mmu.h"

void dbg_print_regs(struct gb_state *s) {
    printf("\n\tAF\tBC\tDE\tHL\tSP\tPC\t\tLY\tZNHC\n");
    printf("\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t\t%04x\t%d%d%d%d\n",
            s->reg16.AF, s->reg16.BC, s->reg16.DE, s->reg16.HL, s->sp, s->pc,
            s->io_lcd_LY, s->flags.ZF, s->flags.NF, s->flags.HF, s->flags.CF);
}

int dbg_run_debugger(struct gb_state *s) {
    (void)s;
    return 1; /* Exit */
}
