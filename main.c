#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include "types.h"
#include "hwdefs.h"
#include "emu.h"
#include "state.h"
#include "cpu.h"
#include "mmu.h"
#include "lcd.h"
#include "audio.h"
#include "disassembler.h"
#include "debugger.h"
#include "gui.h"
#include "fileio.h"

#define GUI_WINDOW_TITLE "KoenGB"
#define GUI_ZOOM      4


void print_usage(char *progname) {
    printf("Usage: %s [option]... rom\n\n", progname);
    printf("GameBoy emulator by koenk.\n\n");
    printf("Options:\n");
    printf(" -S, --break-start      Break into debugger before executing first "
            "instruction.\n");
    printf(" -a, --audio            Enable (WIP) audio\n");
    printf(" -d, --print-disas      Print every instruction before executing "
            "it.\n");
    printf(" -m, --print-mmu        Print every memory access\n");
    printf(" -b, --bios=FILE        Use the specified bios (default is no "
            "bios).\n");
    printf(" -l, --load-state=FILE  Load the gamestate from a file (makes ROM "
            "optional).\n");
    printf(" -e, --load-save=FILE   Load the battery-backed (cartridge) RAM "
            "from a file, the \n");
    printf("                        normal way of saving games. (Optional: the "
            "emulator will \n");
    printf("                        automatically search for this file).\n");
}

int parse_args(int argc, char **argv, struct emu_args *emu_args) {
    memset(emu_args, 0, sizeof(struct emu_args));

    if (argc == 1) {
        print_usage(argv[0]);
        return 1;
    }

    while (1) {
        static struct option long_options[] = {
            {"break-start",  no_argument,        0,  'S'},
            {"audio",        no_argument,        0,  'a'},
            {"print-disas",  no_argument,        0,  'd'},
            {"print-mmu",    no_argument,        0,  'm'},
            {"bios",         required_argument,  0,  'b'},
            {"load-state",   required_argument,  0,  'l'},
            {"load-save",    required_argument,  0,  'e'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "Sadmb:l:e:", long_options, NULL);

        if (c == -1)
            break;

        switch (c) {
            case 'S':
                emu_args->break_at_start = 1;
                break;

            case 'a':
                emu_args->audio_enable = 1;
                break;

            case 'd':
                emu_args->print_disas = 1;
                break;

            case 'm':
                emu_args->print_mmu = 1;
                break;

            case 'b':
                emu_args->bios_filename = optarg;
                break;

            case 'l':
                emu_args->state_filename = optarg;
                break;

            case 'e':
                emu_args->save_filename = optarg;
                break;

            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (optind != argc - 1) {
        /* The remainder are non-option arguments (ROM) */
        print_usage(argv[0]);
        return 1;
    }

    emu_args->rom_filename = argv[optind];

    return 0;
}



int main(int argc, char *argv[]) {
    struct gb_state gb_state;

    struct emu_args emu_args;
    if (parse_args(argc, argv, &emu_args))
        return 1;

    if (emu_init(&gb_state, &emu_args)) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }

    /* Initialize frontend-specific GUI */
    if (gui_lcd_init(GB_LCD_WIDTH, GB_LCD_HEIGHT, GUI_ZOOM, GUI_WINDOW_TITLE)) {
        fprintf(stderr, "Couldn't initialize GUI LCD\n");
        return 1;
    }
    if (emu_args.audio_enable) {
        if (gui_audio_init(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS, AUDIO_SNDBUF_SIZE,
                    gb_state.emu_state->audio_sndbuf)) {
            fprintf(stderr, "Couldn't initialize GUI audio\n");
            return 1;
        }
    }

    printf("==========================\n");
    printf("=== Starting execution ===\n");
    printf("==========================\n\n");

    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    struct player_input input_state;
    memset(&input_state, 0, sizeof(struct player_input));

    while (!gb_state.emu_state->quit) {
        emu_step_frame(&gb_state);

        gui_input_poll(&input_state);
        emu_process_inputs(&gb_state, &input_state);

        gui_lcd_render_frame(gb_state.gb_type == GB_TYPE_CGB,
                gb_state.emu_state->lcd_pixbuf);

        if (gb_state.emu_state->audio_enable) /* TODO */
            audio_update(&gb_state);
    }

    if (gb_state.emu_state->extram_dirty)
        emu_save(&gb_state, 1, gb_state.emu_state->save_filename_out);

    gettimeofday(&endtime, NULL);

    printf("\nEmulation ended at instr: ");
    disassemble(&gb_state);
    dbg_print_regs(&gb_state);

    int t_usec = endtime.tv_usec - starttime.tv_usec;
    int t_sec = endtime.tv_sec - starttime.tv_sec;
    double exectime = t_sec + (t_usec / 1000000.);

    double emulated_secs = gb_state.emu_state->time_seconds +
        gb_state.emu_state->time_cycles / 4194304.;

    printf("\nEmulated %f sec in %f sec WCT, %.0f%%.\n", emulated_secs, exectime,
            emulated_secs / exectime * 100);

    return 0;
}
