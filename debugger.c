#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
    static char last_exec_cmd = 0;

    printf("Break, next instruction: ");
    disassemble(s);

    while (1) {
        char *raw_input = readline("(GBd) ");
        if (!raw_input) /* EOF */
            return 1;
        add_history(raw_input);

        /* Copy into buffer that is automatically free'd on function exit. */
        char input[32];
        strncpy(input, raw_input, sizeof(input));
        input[sizeof(input) - 1] = '\0';
        free(raw_input);

        if (strlen(input) == 0) {
            if (last_exec_cmd == 's')
                s->emu_state->dbg_break_next = 1;
            else if (last_exec_cmd == 'c')
                s->emu_state->dbg_break_next = 0;
            else
                continue;
            return 0;
        }


        switch (input[0]) {
        case 'r': /* Regs - Print all regs */
        case 'p':
            dbg_print_regs(s);
            break;

        case 'x': /* eXamine - dump memory of given address */
        {
            u16 loc = strtol(&input[2], NULL, 16);
            printf("%.4x: %.2x\n", loc, mmu_read(s, loc));
            break;
        }
        case 'd': /* Disassemble - print instruction at given location */
        {
            u16 loc = strtol(&input[2], NULL, 16);
            disassemble_pc(s, loc);
            break;
        }
        case 's': /* Step - execute one instruction */
            s->emu_state->dbg_break_next = 1;
            last_exec_cmd = 's';
            return 0;

        case 'c': /* Continue - continue execution until breakpoint */
            s->emu_state->dbg_break_next = 0;
            last_exec_cmd = 'c';
            return 0;

        case 'b': /* Breakpoint - place new breakpoint */
        {
            u16 loc = strtol(&input[2], NULL, 16);
            s->emu_state->dbg_breakpoint = loc;
            break;
        }
        case 'q': /* Quit */
            s->emu_state->quit = 1;
            return 1;

        case 'h': /* Help */
            printf("GBd debugger commands:\n");
            printf(" r     - [P]rint all [r]egisters (alias: p)\n");
            printf(" x loc - E[x]amine memory at `loc`\n");
            printf(" d loc - [D]isassemble memory at `loc`\n");
            printf(" b loc - Place a [b]reakpoint at PC `loc`\n");
            printf(" s     - [S]tep: execute single instruction\n");
            printf(" c     - [C]ontinue executiong until next breakpoint\n");
            printf(" q     - [Q]uit emulator\n");
            printf(" h     - Show [h]elp\n");
            break;

        default:
            printf("Unknown command: '%s'\n", input);
            break;
        }
    }
}
