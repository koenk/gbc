#include <stdlib.h>
#include <stdio.h>

// Windows has no gettimeofday() (or sys/time.h for that matter).
#ifdef _MSC_VER
    #include "wintime.h"
#else
    #include <sys/time.h>
#endif

#include "types.h"
#include "cpu.h"
#include "disassembler.h"

void print_rom_header_info(u8* rom) {
    printf("Title: %s\n", &rom[0x134]);

    printf("Cart type: ");
    u8 cart_type = rom[0x147];
    switch(cart_type) {
    case 0x00: printf("ROM ONLY\n"); break;
    case 0x01: printf("MBC1\n"); break;
    case 0x02: printf("MBC1+RAM\n"); break;
    case 0x03: printf("MBC1+RAM+BATTERY\n"); break;
    case 0x04: printf("MBC2\n"); break;
    case 0x06: printf("MBC2+BATTERY\n"); break;
    case 0x08: printf("ROM+RAM\n"); break;
    case 0x09: printf("ROM+RAM+BATTERY\n"); break;
    case 0x0b: printf("MMM01\n"); break;
    case 0x0c: printf("MMM01+RAM\n"); break;
    case 0x0d: printf("MMM01+RAM+BATTERY\n"); break;
    case 0x0f: printf("MBC3+TIMER+BATTERY\n"); break;
    case 0x10: printf("MBC3+TIMER+RAM+BATTERY\n"); break;
    case 0x11: printf("MBC3\n"); break;
    case 0x12: printf("MBC3+RAM\n"); break;
    case 0x13: printf("MBC3+RAM+BATTERY\n"); break;
    case 0x15: printf("MBC4\n"); break;
    case 0x16: printf("MBC4+RAM\n"); break;
    case 0x17: printf("MBC4+RAM+BATTERY\n"); break;
    case 0x19: printf("MBC5\n"); break;
    case 0x1a: printf("MBC5+RAM\n"); break;
    case 0x1b: printf("MBC5+RAM+BATTERY\n"); break;
    case 0x1c: printf("MBC5+RUMBLE\n"); break;
    case 0x1d: printf("MBC5+RUMBLE+RAM\n"); break;
    case 0x1e: printf("MBC5+RUMBLE+RAM+BATTERY\n"); break;
    case 0xfc: printf("POCKET CAMERA\n"); break;
    case 0xfd: printf("BANDAI TAMA5\n"); break;
    case 0xfe: printf("HuC3\n"); break;
    case 0xff: printf("HuC1+RAM+BATTERY\n"); break;
    }

    printf("ROM Size: ");
    u8 cart_ROM = rom[0x148];
    switch(cart_ROM) {
    case 0x00: printf("32KB (no banks)\n"); break;
    case 0x01: printf("64KB (4 banks)\n"); break;
    case 0x02: printf("128KB (8 banks)\n"); break;
    case 0x03: printf("256KB (16 banks)\n"); break;
    case 0x04: printf("512KB (32 banks)\n"); break;
    case 0x05: printf("1MB (64 banks)\n"); break;
    case 0x06: printf("2MB (128 banks)\n"); break;
    case 0x07: printf("4MB (256 banks)\n"); break;
    case 0x52: printf("1.1MB (72 banks)\n"); break;
    case 0x53: printf("1.2MB (80 banks)\n"); break;
    case 0x54: printf("1.5MB (96 banks)\n"); break;
    }

    printf("RAM Size: ");
    u8 cart_RAM = rom[0x149];
    switch(cart_RAM) {
    case 0x00: printf("None\n"); break;
    case 0x01: printf("2KB\n"); break;
    case 0x02: printf("8KB\n"); break;
    case 0x03: printf("32KB (4 banks)\n"); break;
    }
}


void read_file(char *filename, u8 **src, size_t *size) {
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to load file (\"%s\").\n", filename);
        #ifdef _MSC_VER
            __debugbreak();
        #endif
        exit(1);
    }

    // get the file size
    fseek(fp, 0L, SEEK_END);
    long allocsize = ftell(fp);
    rewind(fp);

    *src = (u8 *)malloc(allocsize * sizeof(u8));
    if (*src == NULL) {
        printf("Error while allocating memory for reading file.");
        fclose(fp);
        return;
    }
    *size = fread(*src, sizeof(u8), allocsize, fp);
    fclose(fp);
}

int main(void) {
    //char filename[] = "pkmn_blue.gb"; int rom_type = 0;
    //char filename[] = "pkmn_silver.gbc"; int rom_type = 1;
    char filename[] = "test.gb"; int rom_type = 0;
    struct gb_state *gb_state;
    u8 *file;
    size_t filesize;

    read_file(filename, &file, &filesize);


    gb_state = malloc(sizeof(struct gb_state));
    gb_state->rom = file;
    gb_state->rom_type = rom_type;

    print_rom_header_info(file);

    disassemble_bootblock(gb_state);

    printf("==========================\n");
    printf("=== Starting execution ===\n");
    printf("==========================\n\n");

    printf("regs: %p\n", &gb_state->regs);
    printf("r8 %p %p %p %p %p %p %p %p\n",
            &gb_state->reg8.B, &gb_state->reg8.C,
            &gb_state->reg8.D, &gb_state->reg8.E,
            &gb_state->reg8.H, &gb_state->reg8.L,
            &gb_state->reg8.A, &gb_state->reg8.F);

    int ret = 0;
    int instr = 0;
    int seconds_to_emulate, cycles_to_emulate;
    cpu_reset_state(gb_state);
    seconds_to_emulate = 100;
    cycles_to_emulate = 4194304 * seconds_to_emulate;
    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    while (!ret && gb_state->cycles < cycles_to_emulate) {
        //if (gb_state->pc > 0x6f && (gb_state->pc < 0x1f80 || gb_state->pc > 0x1f86) && (gb_state->pc < 0x36e2 || gb_state->pc > 0x36e7)) {
            //emu.print_regs();
            disassemble(gb_state);
        //}
        ret = cpu_do_instruction(gb_state);
        instr++;
    }

    gettimeofday(&endtime, NULL);
    printf("\nEmulation ended at instr: ");

    if (ret)
        disassemble(gb_state);

    cpu_print_regs(gb_state);

    int t_usec = endtime.tv_usec - starttime.tv_usec;
    int t_sec = endtime.tv_sec - starttime.tv_sec;
    double exectime = t_sec + (t_usec / 1000000.);

    printf("\nEmulated %f sec (%d instr) in %f sec WCT, %f%%\n", instr / 4194304., instr, exectime, seconds_to_emulate / exectime * 100);

    free(file);

    #ifdef WIN32
        while (1);
    #endif

    return 0;
}
