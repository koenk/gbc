#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>

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


int read_file(char *filename, u8 **buf, size_t *size) {
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to load file (\"%s\").\n", filename);
        return 1;
    }

    /* Get the file size */
    fseek(fp, 0L, SEEK_END);
    long allocsize = ftell(fp) * sizeof(u8);
    rewind(fp);

    *buf = (u8 *)malloc(allocsize);
    if (*buf == NULL) {
        printf("Error allocating mem for file (file=%s, size=%zu byte).",
                filename, allocsize);
        fclose(fp);
        return 1;
    }
    *size = fread(*buf, sizeof(u8), allocsize, fp);
    fclose(fp);
    return 0;
}

int save_file(char *filename, u8 *buf, size_t size) {
    FILE *fp;

    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file (\"%s\").\n", filename);
        return 1;
    }

    fwrite(buf, sizeof(u8), size, fp);
    fclose(fp);
    return 0;
}

struct emu_args {
    char *rom_filename;
    char *bios_filename;
    char *state_filename;
    char *save_filename;
    char break_at_start;
    char print_disas;
    char print_mmu;
    char enable_sound;
};

void print_usage(char *progname) {
    printf("Usage: %s [option]... rom\n\n", progname);
    printf("GameBoy emulator by koenk.\n\n");
    printf("Options:\n");
    printf(" -S, --break-start      Break into debugger before executing first "
            "instruction.\n");
    printf(" -s, --sound            Enable (WIP) sound\n");
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
    emu_args->rom_filename = NULL;
    emu_args->bios_filename = NULL;
    emu_args->state_filename = NULL;
    emu_args->save_filename = NULL;
    emu_args->break_at_start = 0;
    emu_args->print_disas = 0;
    emu_args->print_mmu = 0;
    emu_args->enable_sound = 0;

    if (argc == 1) {
        print_usage(argv[0]);
        return 1;
    }

    while (1) {
        static struct option long_options[] = {
            {"break-start",  no_argument,        0,  'S'},
            {"sound",        no_argument,        0,  's'},
            {"print-disas",  no_argument,        0,  'd'},
            {"print-mmu",    no_argument,        0,  'm'},
            {"bios",         required_argument,  0,  'b'},
            {"load-state",   required_argument,  0,  'l'},
            {"load-save",    required_argument,  0,  'e'},
            {0, 0, 0, 0}
        };

        char c = getopt_long(argc, argv, "Ssdmb:l:e:", long_options, NULL);

        if (c == -1)
            break;

        switch (c) {
            case 'S':
                emu_args->break_at_start = 1;
                break;

            case 's':
                emu_args->enable_sound = 1;
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


void save(struct gb_state *s, char extram, char *out_filename) {
    u8 *state_buf;
    size_t state_buf_size;

    if (extram && !s->has_extram)
        return;

    if (extram)
        state_save_extram(s, &state_buf, &state_buf_size);
    else
        state_save(s, &state_buf, &state_buf_size);

    save_file(out_filename, state_buf, state_buf_size);

    printf("%s saved to \"%s\".\n", extram ? "Ext RAM" : "State", out_filename);
}


int main(int argc, char *argv[]) {
    struct gb_state gb_state;
    char *rom_filename = NULL;
    char state_filename_out[1024];
    char save_filename_out[1024];

    struct emu_args emu_args;
    if (parse_args(argc, argv, &emu_args))
        exit(1);

    if (emu_args.state_filename) {
        printf("Loading savestate from \"%s\" ...\n", emu_args.state_filename);
        u8 *state_buf;
        size_t state_buf_size;
        if (read_file(emu_args.state_filename, &state_buf, &state_buf_size)) {
            fprintf(stderr, "Error during reading of state file \"%s\".\n",
                    emu_args.state_filename);
            exit(1);
        }
        if (state_load(&gb_state, state_buf, state_buf_size)) {
            fprintf(stderr, "Error during loading of state, aborting.\n");
            exit(1);
        }
        print_rom_header_info(gb_state.mem_ROM);

    } else {
        u8 *rom;
        size_t rom_size;
        printf("Loading ROM \"%s\"\n", emu_args.rom_filename);
        if (read_file(emu_args.rom_filename, &rom, &rom_size)) {
            fprintf(stderr, "Error during reading of ROM file \"%s\".\n",
                    emu_args.rom_filename);
            exit(1);
        }

        print_rom_header_info(rom);

        if (state_new_from_rom(&gb_state, rom, rom_size)) {
            fprintf(stderr, "Error loading ROM \"%s\", aborting.\n",
                    emu_args.rom_filename);
            exit(1);
        }

        cpu_reset_state(&gb_state);

        if (emu_args.bios_filename) {
            u8 *bios;
            size_t bios_size;
            read_file("bios.bin", &bios, &bios_size);
            state_add_bios(&gb_state, bios, bios_size);
        }

        if (emu_args.save_filename) {
            printf("Loading save from \"%s\" ...\n", emu_args.save_filename);
            u8 *state_buf;
            size_t state_buf_size;
            if (read_file(emu_args.save_filename, &state_buf, &state_buf_size)) {
                fprintf(stderr, "Error during reading of save file \"%s\".\n",
                        emu_args.save_filename);
                exit(1);
            }
            if (state_load_extram(&gb_state, state_buf, state_buf_size)) {
                fprintf(stderr, "Error during loading of save, aborting.\n");
                exit(1);
            }
        } else {
            snprintf(save_filename_out, sizeof(save_filename_out), "%ssav",
                    emu_args.rom_filename);
            u8 *state_buf;
            size_t state_buf_size;
            if (read_file(save_filename_out, &state_buf, &state_buf_size) == 0)
                if (state_load_extram(&gb_state, state_buf, state_buf_size)) {
                    fprintf(stderr, "Error during loading of save.\n");
                    exit(1);
                }

        }
    }

    if (emu_args.rom_filename)
        rom_filename = emu_args.rom_filename;

    snprintf(save_filename_out, sizeof(save_filename_out), "%ssav",
            rom_filename);
    snprintf(state_filename_out, sizeof(state_filename_out), "%sstate",
            rom_filename);

    init_emu_state(&gb_state);
    cpu_init_emu_cpu_state(&gb_state);

    if (gui_init()) {
        printf("Couldn't initialize LCD, exiting...\n");
        return 1;
    }

    if (emu_args.enable_sound)
        if (audio_init(&gb_state)) {
            printf("Couldn't initialize audio, exiting...\n");
            return 1;
        }

    if (emu_args.break_at_start)
        gb_state.emu_state->dbg_break_next = 1;
    if (emu_args.print_disas)
        gb_state.emu_state->dbg_print_disas = 1;
    if (emu_args.print_mmu)
        gb_state.emu_state->dbg_print_mmu = 1;
    if (emu_args.enable_sound)
        gb_state.emu_state->enable_sound = 1;


    printf("==========================\n");
    printf("=== Starting execution ===\n");
    printf("==========================\n\n");

    int ret = 0;
    int instr = 0;
    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    while (!ret && !gb_state.emu_state->quit) {
        if (gb_state.emu_state->dbg_print_disas)
            disassemble(&gb_state);

        if (gb_state.emu_state->dbg_break_next ||
            gb_state.pc == gb_state.emu_state->dbg_breakpoint)
            if (dbg_run_debugger(&gb_state))
                break;

        ret = cpu_do_instruction(&gb_state);
        instr++;

        if (gb_state.emu_state->lcd_line_needs_rerender) {
            gui_render_current_line(&gb_state);
            gb_state.emu_state->lcd_line_needs_rerender = 0;
        }

        if (gb_state.emu_state->lcd_screen_needs_rerender) {
            gui_render_frame(&gb_state);
            gb_state.emu_state->lcd_screen_needs_rerender = 0;

            if (gui_handleinputs(&gb_state))
                break;

            /* Save periodically (once per frame) if dirty. */
            if (gb_state.emu_state->extram_dirty)
                save(&gb_state, 1, save_filename_out);
            gb_state.emu_state->extram_dirty = 0;

            /* TODO */
            if (gb_state.emu_state->enable_sound)
                audio_update(&gb_state);
        }


        if (gb_state.emu_state->make_savestate) {
            gb_state.emu_state->make_savestate = 0;
            save(&gb_state, 0, state_filename_out);
        }

        if (gb_state.emu_state->flush_extram) {
            gb_state.emu_state->flush_extram = 0;
            if (gb_state.emu_state->extram_dirty)
                save(&gb_state, 1, save_filename_out);
            gb_state.emu_state->extram_dirty = 0;
        }

    }

    gettimeofday(&endtime, NULL);
    printf("\nEmulation ended at instr: ");

    if (ret)
        disassemble(&gb_state);

    dbg_print_regs(&gb_state);

    int t_usec = endtime.tv_usec - starttime.tv_usec;
    int t_sec = endtime.tv_sec - starttime.tv_sec;
    double exectime = t_sec + (t_usec / 1000000.);
    double emulated_secs = instr / 4194304.;

    printf("\nEmulated %f sec (%d instr) in %f sec WCT, %f%%.\n", emulated_secs,
            instr, exectime,  emulated_secs / exectime * 100);

    #ifdef WIN32
        while (1);
    #endif

    return 0;
}
