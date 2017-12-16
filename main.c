#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Windows has no gettimeofday() (or sys/time.h for that matter). */
#ifdef _MSC_VER
    #include "wintime.h"
#else
    #include <sys/time.h>
#endif

#include "types.h"
#include "state.h"
#include "cpu.h"
#include "mmu.h"
#include "disassembler.h"
#include "gui.h"
#include "debugger.h"


int main(int argc, char *argv[]) {
    struct gb_state *gb_state;
    u8 *rom;
    size_t romsize;
    u8 *bios;
    size_t bios_size;

    char *romname = "test.gb";
    enum gb_type gb_type = GB_TYPE_GB;
    if (argc > 3) {
        printf("Usage: %s [rom] [state]\n", argv[0]);
        exit(1);
    }

    if (argc >= 2)
        romname = argv[1];

    if (argc != 3) {
        read_file(romname, &rom, &romsize);
        read_file("bios.bin", &bios, &bios_size);
        assert(bios_size == 256);

        print_rom_header_info(rom);

        gb_state = new_gb_state(bios, rom, romsize, gb_type);

        disassemble_bootblock(gb_state);

        cpu_reset_state(gb_state);
    } else {
        printf("Loading savestate ...\n");
        gb_state = malloc(sizeof(struct gb_state));
        if (state_load("koekje.gbstate", gb_state)) {
            fprintf(stderr, "Error during loading of state, aborting...\n");
            exit(1);
        }
        print_rom_header_info(gb_state->mem_ROM);
    }

    init_emu_state(gb_state);
    cpu_init_emu_cpu_state(gb_state);

    if (gui_init()) {
        printf("Couldn't initialize LCD, exiting...\n");
        return 1;
    }

    printf("==========================\n");
    printf("=== Starting execution ===\n");
    printf("==========================\n\n");

    int ret = 0;
    int instr = 0;
    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

#if 0
    if (argc == 3)
        gb_state->dbg_break_next = 1;
#endif

    while (!ret && !gb_state->emu_state->quit) {
        //disassemble(gb_state);

        if (gb_state->emu_state->dbg_break_next ||
            gb_state->pc == gb_state->emu_state->dbg_breakpoint)
            if (dbg_run_debugger(gb_state))
                break;

        ret = cpu_do_instruction(gb_state);
        instr++;

        if (gb_state->lcd_line_needs_rerender) {
            gui_render_current_line(gb_state);
            gb_state->lcd_line_needs_rerender = 0;
        }

        if (gb_state->lcd_screen_needs_rerender) {
            gui_render_frame(gb_state);
            gb_state->lcd_screen_needs_rerender = 0;

            if (gui_handleinputs(gb_state))
                break;
        }

        if (gb_state->emu_state->make_savestate) {
            gb_state->emu_state->make_savestate = 0;
            state_save("koekje.gbstate", gb_state);
            printf("State saved to \"%s\"\n", "koekje.gbstate");
        }

    }

    gettimeofday(&endtime, NULL);
    printf("\nEmulation ended at instr: ");

    if (ret)
        disassemble(gb_state);

    dbg_print_regs(gb_state);

    int t_usec = endtime.tv_usec - starttime.tv_usec;
    int t_sec = endtime.tv_sec - starttime.tv_sec;
    double exectime = t_sec + (t_usec / 1000000.);
    double emulated_secs = instr / 4194304.;

    printf("\nEmulated %f sec (%d instr) in %f sec WCT, %f%%\n", emulated_secs,
            instr, exectime,  emulated_secs / exectime * 100);

    free(gb_state);
    free(rom);
    free(bios);

    #ifdef WIN32
        while (1);
    #endif

    return 0;
}
