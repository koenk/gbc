#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include "types.h"
#include "hwdefs.h"
#include "state.h"
#include "cpu.h"
#include "mmu.h"
#include "lcd.h"
#include "audio.h"
#include "disassembler.h"
#include "debugger.h"
#include "gui.h"

#define GUI_WINDOW_TITLE "KoenGB"
#define GUI_ZOOM      4

#define emu_error(fmt, ...) \
    do { \
        printf("Emu initialization error: " fmt "\n", ##__VA_ARGS__); \
        return 1; \
    } while (0)

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
    char audio_enable;
};

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

        char c = getopt_long(argc, argv, "Sadmb:l:e:", long_options, NULL);

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


void process_inputs(struct gb_state *s) {
    struct gui_input input;
    while (gui_input_poll(&input)) {
        switch (input.type) {
        case GUI_INPUT_EXIT:
            s->emu_state->quit = 1;

        case GUI_INPUT_DBGBREAK:
            s->emu_state->dbg_break_next = 1;
            break;

        case GUI_INPUT_SAVESTATE:
            s->emu_state->make_savestate = 1;
            break;

        case GUI_INPUT_BUTTON_DOWN:
            switch (input.button) {
            case GUI_BUTTON_START:  s->io_buttons_buttons &= ~(1<<3); break;
            case GUI_BUTTON_SELECT: s->io_buttons_buttons &= ~(1<<2); break;
            case GUI_BUTTON_B:      s->io_buttons_buttons &= ~(1<<1); break;
            case GUI_BUTTON_A:      s->io_buttons_buttons &= ~(1<<0); break;
            case GUI_BUTTON_DOWN:   s->io_buttons_dirs    &= ~(1<<3); break;
            case GUI_BUTTON_UP:     s->io_buttons_dirs    &= ~(1<<2); break;
            case GUI_BUTTON_LEFT:   s->io_buttons_dirs    &= ~(1<<1); break;
            case GUI_BUTTON_RIGHT:  s->io_buttons_dirs    &= ~(1<<0); break;
            }
            break;

        case GUI_INPUT_BUTTON_UP:
            switch (input.button) {
            case GUI_BUTTON_START:  s->io_buttons_buttons |= 1<<3; break;
            case GUI_BUTTON_SELECT: s->io_buttons_buttons |= 1<<2; break;
            case GUI_BUTTON_B:      s->io_buttons_buttons |= 1<<1; break;
            case GUI_BUTTON_A:      s->io_buttons_buttons |= 1<<0; break;
            case GUI_BUTTON_DOWN:   s->io_buttons_dirs    |= 1<<3; break;
            case GUI_BUTTON_UP:     s->io_buttons_dirs    |= 1<<2; break;
            case GUI_BUTTON_LEFT:   s->io_buttons_dirs    |= 1<<1; break;
            case GUI_BUTTON_RIGHT:  s->io_buttons_dirs    |= 1<<0; break;
            }
            break;
        case GUI_INPUT_NONE:
            break;
        }
    }
}

int emu_init(struct gb_state *s, struct emu_args *args) {
    memset(s, 0, sizeof(struct gb_state));

    init_emu_state(s);
    cpu_init_emu_cpu_state(s);

    if (!args->rom_filename)
        emu_error("Must specify ROM filename");
    if (strlen(args->rom_filename) >
            sizeof(s->emu_state->state_filename_out) - 6)
        emu_error("ROM filename too long (%s)", args->rom_filename);

    if (args->state_filename) {
        printf("Loading savestate from \"%s\" ...\n", args->state_filename);
        u8 *state_buf;
        size_t state_buf_size;
        if (read_file(args->state_filename, &state_buf, &state_buf_size))
            emu_error("Error during reading of state file \"%s\".\n",
                    args->state_filename);

        if (state_load(s, state_buf, state_buf_size))
            emu_error("Error during loading of state, aborting.\n");

        print_rom_header_info(s->mem_ROM);

    } else {
        u8 *rom;
        size_t rom_size;
        printf("Loading ROM \"%s\"\n", args->rom_filename);
        if (read_file(args->rom_filename, &rom, &rom_size))
            emu_error("Error during reading of ROM file \"%s\".\n",
                    args->rom_filename);

        print_rom_header_info(rom);

        if (state_new_from_rom(s, rom, rom_size))
            emu_error("Error loading ROM \"%s\", aborting.\n",
                    args->rom_filename);

        cpu_reset_state(s);

        if (args->bios_filename) {
            u8 *bios;
            size_t bios_size;
            read_file(args->bios_filename, &bios, &bios_size);
            state_add_bios(s, bios, bios_size);
        }

        if (args->save_filename) {
            u8 *state_buf;
            size_t state_buf_size;
            if (read_file(args->save_filename, &state_buf, &state_buf_size))
                emu_error("Error during reading of save file \"%s\".",
                        args->save_filename);

            if (state_load_extram(s, state_buf, state_buf_size))
                emu_error("Error during loading of save, aborting.\n");
        } else {
            snprintf(s->emu_state->save_filename_out,
                    sizeof(s->emu_state->save_filename_out), "%ssav",
                    args->rom_filename);
            u8 *state_buf;
            size_t state_buf_size;
            if (read_file(s->emu_state->save_filename_out, &state_buf,
                        &state_buf_size) == 0)
                if (state_load_extram(s, state_buf, state_buf_size))
                    emu_error("Error during loading of save.\n");

        }
    }

    snprintf(s->emu_state->save_filename_out,
            sizeof(s->emu_state->save_filename_out), "%ssav",
            args->rom_filename);
    snprintf(s->emu_state->state_filename_out,
            sizeof(s->emu_state->state_filename_out), "%sstate",
            args->rom_filename);

    if (lcd_init(s))
        emu_error("Couldn't initialize LCD");
    if (gui_lcd_init(GB_LCD_WIDTH, GB_LCD_HEIGHT, GUI_ZOOM, GUI_WINDOW_TITLE))
        emu_error("Couldn't initialize GUI LCD");

    if (args->audio_enable) {
        if (audio_init(s))
            emu_error("Couldn't initialize audio");
        if (gui_audio_init(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS, AUDIO_SNDBUF_SIZE,
                    s->emu_state->audio_sndbuf))
            emu_error("Couldn't initialize GUI audio");
    }

    if (args->break_at_start)
        s->emu_state->dbg_break_next = 1;
    if (args->print_disas)
        s->emu_state->dbg_print_disas = 1;
    if (args->print_mmu)
        s->emu_state->dbg_print_mmu = 1;
    if (args->audio_enable)
        s->emu_state->audio_enable = 1;
    return 0;
}

void emu_step(struct gb_state *s) {
    if (s->emu_state->dbg_print_disas)
        disassemble(s);

    if (s->emu_state->dbg_break_next ||
        s->pc == s->emu_state->dbg_breakpoint)
        if (dbg_run_debugger(s)) {
            s->emu_state->quit = 1;
            return;
        }

    cpu_step(s);
    lcd_step(s);
    mmu_step(s);
    cpu_timers_step(s);

    s->emu_state->time_cycles += s->emu_state->last_op_cycles;
    if (s->emu_state->time_cycles >= GB_FREQ) {
        s->emu_state->time_cycles %= GB_FREQ;
        s->emu_state->time_seconds++;
    }

    if (s->emu_state->lcd_entered_vblank) {
        gui_lcd_render_frame(s->gb_type == GB_TYPE_CGB,
                s->emu_state->lcd_pixbuf);

        process_inputs(s);

        if (s->emu_state->audio_enable) /* TODO */
            audio_update(s);

        /* Save periodically (once per frame) if dirty. */
        s->emu_state->flush_extram = 1;
    }

    if (s->emu_state->make_savestate) {
        s->emu_state->make_savestate = 0;
        save(s, 0, s->emu_state->state_filename_out);
    }

    if (s->emu_state->flush_extram) {
        s->emu_state->flush_extram = 0;
        if (s->emu_state->extram_dirty)
            save(s, 1, s->emu_state->save_filename_out);
        s->emu_state->extram_dirty = 0;
    }
}


int main(int argc, char *argv[]) {
    struct gb_state gb_state;

    struct emu_args emu_args;
    if (parse_args(argc, argv, &emu_args))
        return 1;

    if (emu_init(&gb_state, &emu_args))
        emu_error("Initialization failed");


    printf("==========================\n");
    printf("=== Starting execution ===\n");
    printf("==========================\n\n");

    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    while (!gb_state.emu_state->quit) {
        emu_step(&gb_state);
    }

    if (gb_state.emu_state->extram_dirty)
        save(&gb_state, 1, gb_state.emu_state->save_filename_out);

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
